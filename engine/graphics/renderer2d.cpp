#include "renderer2d.h"
#include "engine/graphics/texture_asset.h"
#include "engine/core/log.h"
#include "rlgl.h"

namespace CHEngine
{
    Renderer2D* Renderer2D::s_Instance = nullptr;

    void Renderer2D::Init()
    {
        CH_CORE_ASSERT(!s_Instance, "Renderer2D already initialized!");
        s_Instance = new Renderer2D();
        CH_CORE_INFO("Initializing Renderer2D (Batching Mode)...");
    }

    void Renderer2D::Shutdown()
    {
        CH_CORE_INFO("Shutting down Renderer2D...");
        delete s_Instance;
        s_Instance = nullptr;
    }

    Renderer2D::Renderer2D()
    {
        m_Data = std::make_unique<Renderer2DData>();
        m_Data->QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];

        // Create 1x1 white texture for plain quads
        Image whiteImage = GenImageColor(1, 1, WHITE);
        m_Data->TextureSlots[0] = LoadTextureFromImage(whiteImage);
        UnloadImage(whiteImage);
    }

    Renderer2D::~Renderer2D()
    {
        UnloadTexture(m_Data->TextureSlots[0]);
        delete[] m_Data->QuadVertexBufferBase;
    }

    void Renderer2D::BeginCanvas()
    {
        // UI rendering uses screen coordinates
    }

    void Renderer2D::EndCanvas()
    {
        Flush();
    }

    void Renderer2D::BeginScene(const Camera2D& camera)
    {
        BeginMode2D(camera);
        StartBatch();
    }

    void Renderer2D::EndScene()
    {
        Flush();
        EndMode2D();
    }

    void Renderer2D::StartBatch()
    {
        m_Data->QuadIndexCount = 0;
        m_Data->QuadVertexBufferPtr = m_Data->QuadVertexBufferBase;
        m_Data->TextureSlotIndex = 1;
    }

    void Renderer2D::Flush()
    {
        if (m_Data->QuadIndexCount == 0)
            return;

        uint32_t quadCount = (uint32_t)(m_Data->QuadVertexBufferPtr - m_Data->QuadVertexBufferBase) / 4;
        
        // For now, since we don't have a multi-sampler shader, we group by texture.
        // But the DrawSprite already ensures we flush on texture change.
        // So here we assume all quads in the current batch use the textures in slots
        // actually, to keep it simple with rlgl:
        // We only support ONE texture per batch for now in this custom batcher 
        // OR we use rlgl's internal mechanism.
        
        // If we want to be "Hazel-style", we should bind texture slots.
        // Since rlgl is a global state machine, we can't easily bind 32 textures 
        // without a custom shader.
        
        // So: let's just draw the damn quads.
        rlBegin(RL_QUADS);
        for (uint32_t i = 0; i < quadCount * 4; i++)
        {
            const auto& v = m_Data->QuadVertexBufferBase[i];
            rlColor4ub(v.Color.r, v.Color.g, v.Color.b, v.Color.a);
            rlTexCoord2f(v.TexCoord.x, v.TexCoord.y);
            rlVertex3f(v.Position.x, v.Position.y, v.Position.z);
        }
        rlEnd();
        rlDisableTexture();

        m_Data->Stats.DrawCalls++;
        StartBatch();
    }

    void Renderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, Color color)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, color);
    }

    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, Color color)
    {
        if (m_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
            NextBatch();

        if (m_Data->QuadIndexCount == 0)
        {
            m_Data->TextureSlotIndex = 0; // Use white texture
            rlEnableTexture(m_Data->TextureSlots[0].id);
        }
        else if (m_Data->TextureSlotIndex != 0)
        {
            // If we were drawing sprites with a non-white texture, flush
            NextBatch();
            m_Data->TextureSlotIndex = 0;
            rlEnableTexture(m_Data->TextureSlots[0].id);
        }

        // 0,0 origin for now
        m_Data->QuadVertexBufferPtr->Position = position;
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {0, 0};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {position.x + size.x, position.y, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {1, 0};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {position.x + size.x, position.y + size.y, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {1, 1};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {position.x, position.y + size.y, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {0, 1};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadIndexCount += 6;
        m_Data->Stats.QuadCount++;
    }

    void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, float rotation, Color color)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, rotation, color);
    }

    void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, float rotation, Color color)
    {
        // For now, if rotation is present, we flush and use Raylib's legacy call to avoid complex vertex math here
        // until we have a proper Matrix vertex transform in the batcher.
        if (rotation == 0.0f)
        {
            DrawQuad(position, size, color);
            return;
        }

        Flush();
        DrawRectanglePro({position.x, position.y, size.x, size.y}, {size.x * 0.5f, size.y * 0.5f}, rotation, color);
        m_Data->Stats.QuadCount++;
        m_Data->Stats.DrawCalls++;
    }

    void Renderer2D::DrawSprite(const Vector3& position, const Vector2& size, const std::shared_ptr<TextureAsset>& texture, Color tint)
    {
        if (!texture || !texture->IsReady())
        {
            DrawQuad(position, size, tint);
            return;
        }

        Texture2D tex = texture->GetTexture();

        // If we have a different texture than the one currently in the batch, flush
        if (m_Data->QuadIndexCount > 0 && (m_Data->TextureSlotIndex == 0 || m_Data->TextureSlots[0].id != tex.id))
        {
            NextBatch();
        }

        if (m_Data->QuadIndexCount == 0)
        {
            m_Data->TextureSlots[0] = tex;
            m_Data->TextureSlotIndex = 1;
            rlEnableTexture(tex.id);
        }

        DrawQuad(position, size, tint);
    }

    void Renderer2D::DrawSprite(const Vector2& position, const Vector2& size, const std::shared_ptr<TextureAsset>& texture, Color tint)
    {
        DrawSprite({position.x, position.y, 0.0f}, size, texture, tint);
    }

    void Renderer2D::DrawSprite(const Vector2& position, const Vector2& size, float rotation, const std::shared_ptr<TextureAsset>& texture, Color tint)
    {
        DrawSprite({position.x, position.y, 0.0f}, size, rotation, texture, tint);
    }

    void Renderer2D::DrawSprite(const Vector3& position, const Vector2& size, float rotation, const std::shared_ptr<TextureAsset>& texture, Color tint)
    {
        if (!texture || !texture->IsReady())
        {
            DrawQuad(position, size, rotation, tint);
            return;
        }

        if (rotation == 0.0f)
        {
            DrawSprite(position, size, texture, tint);
            return;
        }

        // For rotated sprites, use Raylib's Pro call for now
        Flush();
        DrawTexturePro(texture->GetTexture(), 
            {0, 0, (float)texture->GetTexture().width, (float)texture->GetTexture().height},
            {position.x, position.y, size.x, size.y},
            {size.x * 0.5f, size.y * 0.5f}, rotation, tint);
            
        m_Data->Stats.QuadCount++;
        m_Data->Stats.DrawCalls++;
    }

    void Renderer2D::ResetStats()
    {
        memset(&m_Data->Stats, 0, sizeof(Renderer2DData::Statistics));
    }
}
