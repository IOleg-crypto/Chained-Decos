#include "renderer2d.h"
#include "renderer.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/assets/font_asset.h"
#include "render_command.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{
Renderer2D* Renderer2D::s_Instance = nullptr;

Renderer2D& Renderer2D::Get()
{
    CH_CORE_ASSERT(s_Instance, "Renderer2D not initialized!");
    return *s_Instance;
}

void Renderer2D::Init()
{
    if (!s_Instance) s_Instance = new Renderer2D();
    
    CH_CORE_INFO("Initializing Renderer2D (Pure OpenGL Batching)...");

    if (Application::Get().GetSpecification().Headless)
    {
        CH_CORE_INFO("Renderer2D: Headless mode, skipping GL initialization.");
        return;
    }

    auto& m_Data = s_Instance->m_Data;

    m_Data->QuadVertexArray = VertexArray::Create();
    
    m_Data->QuadVertexBuffer = VertexBuffer::Create(Renderer2DData::MaxVertices * sizeof(QuadVertex));
    m_Data->QuadVertexBuffer->SetLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float4, "a_Color" },
        { ShaderDataType::Float2, "a_TexCoord" },
        { ShaderDataType::Float,  "a_TexIndex" }
    });
    m_Data->QuadVertexArray->AddVertexBuffer(m_Data->QuadVertexBuffer);

    uint32_t* quadIndices = new uint32_t[Renderer2DData::MaxIndices];
    uint32_t offset = 0;
    for (uint32_t i = 0; i < Renderer2DData::MaxIndices; i += 6)
    {
        quadIndices[i + 0] = offset + 0;
        quadIndices[i + 1] = offset + 1;
        quadIndices[i + 2] = offset + 2;

        quadIndices[i + 3] = offset + 2;
        quadIndices[i + 4] = offset + 3;
        quadIndices[i + 5] = offset + 0;

        offset += 4;
    }

    std::shared_ptr<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, Renderer2DData::MaxIndices);
    m_Data->QuadVertexArray->SetIndexBuffer(quadIB);
    delete[] quadIndices;

    // Create 1x1 white texture for plain quads using pure OpenGL
    uint32_t whiteTextureData = 0xffffffff;
    glGenTextures(1, &m_Data->TextureSlots[0]);
    glBindTexture(GL_TEXTURE_2D, m_Data->TextureSlots[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whiteTextureData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer2D::Shutdown()
{
    if (s_Instance)
    {
        CH_CORE_INFO("Shutting down Renderer2D...");
        delete s_Instance;
        s_Instance = nullptr;
    }
}

Renderer2D::Renderer2D()
{
    m_Data = std::make_unique<Renderer2DData>();
    m_Data->QuadVertexBufferBase = new QuadVertex[Renderer2DData::MaxVertices];
}

Renderer2D::~Renderer2D()
{
    if (!Application::Get().GetSpecification().Headless)
    {
        glDeleteTextures(1, &m_Data->TextureSlots[0]);
    }
    delete[] m_Data->QuadVertexBufferBase;
}

void Renderer2D::BeginMode2D(const Camera2D& camera)
{
    // Calculate View-Projection matrix for 2D
    // camera.target is the center, camera.offset is the screen position of the center
    // camera.rotation is rotation in degrees, camera.zoom is zoom level
    
    int width = Application::Get().GetWindow().GetWidth();
    int height = Application::Get().GetWindow().GetHeight();
    
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(camera.Offset.x, camera.Offset.y, 0.0f));
    view = glm::rotate(view, glm::radians(camera.Rotation), glm::vec3(0, 0, 1));
    view = glm::scale(view, glm::vec3(camera.Zoom, camera.Zoom, 1.0f));
    view = glm::translate(view, glm::vec3(-camera.Target.x, -camera.Target.y, 0.0f));
    
    m_Data->ViewProjection = projection * view;
}

void Renderer2D::BeginScene(const CHEngine::Camera2D& camera)
{
    BeginMode2D(camera);
    StartBatch();
}

void Renderer2D::EndScene()
{
    Flush();
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
    {
        return;
    }

    uint32_t dataSize = (uint32_t)((uint8_t*)m_Data->QuadVertexBufferPtr - (uint8_t*)m_Data->QuadVertexBufferBase);
    m_Data->QuadVertexBuffer->SetData(m_Data->QuadVertexBufferBase, dataSize);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_Data->TextureSlots[0]);

    // Use a simple 2D shader if available, otherwise just use current
    auto shaderAsset = Renderer::Get().GetShaderLibrary().Exists("Unlit") ? 
                       Renderer::Get().GetShaderLibrary().Get("Unlit") : nullptr;
    if (shaderAsset)
    {
        uint32_t shaderId = shaderAsset->GetShader().id;
        glUseProgram(shaderId);
        glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ViewProjection"), 1, GL_FALSE, glm::value_ptr(m_Data->ViewProjection));
        glUniform1i(glGetUniformLocation(shaderId, "u_Texture"), 0);
    }

    RenderCommand::DrawIndexed(m_Data->QuadVertexArray, m_Data->QuadIndexCount);

    m_Data->Stats.DrawCalls++;
    StartBatch();
}

void Renderer2D::NextBatch()
{
    Flush();
    StartBatch();
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad({position.x, position.y, 0.0f}, size, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
{
    if (m_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
    {
        NextBatch();
    }

    if (m_Data->QuadIndexCount == 0)
    {
        m_Data->TextureSlotIndex = 0; // Use white texture
        m_Data->TextureSlots[0] = s_Instance->m_Data->TextureSlots[0];
    }
    else if (m_Data->TextureSlotIndex != 0)
    {
        NextBatch();
        m_Data->TextureSlotIndex = 0;
    }

    // Vertex data
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

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
    DrawQuad({position.x, position.y, 0.0f}, size, rotation, color);
}

void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
{
    if (rotation == 0.0f)
    {
        DrawQuad(position, size, color);
        return;
    }

    if (m_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
    {
        NextBatch();
    }

    // Matrix calc for rotated quad vertices
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
        * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
        * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

    glm::vec4 quadVertexPositions[4] = {
        { -0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f, 0.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f, 1.0f }
    };

    for (int i = 0; i < 4; i++)
    {
        m_Data->QuadVertexBufferPtr->Position = transform * quadVertexPositions[i];
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = { (i==1||i==2)?1.0f:0.0f, (i==2||i==3)?1.0f:0.0f };
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;
    }

    m_Data->QuadIndexCount += 6;
    m_Data->Stats.QuadCount++;
}

void Renderer2D::DrawSprite(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<TextureAsset>& texture,
                            const glm::vec4& tint)
{
    if (!texture || !texture->IsReady())
    {
        DrawQuad(position, size, tint);
        return;
    }

    uint32_t texId = texture->GetTexture().id;

    if (m_Data->QuadIndexCount > 0 && (m_Data->TextureSlotIndex == 0 || m_Data->TextureSlots[0] != texId))
    {
        NextBatch();
    }

    if (m_Data->QuadIndexCount == 0)
    {
        m_Data->TextureSlots[0] = texId;
        m_Data->TextureSlotIndex = 1;
    }

    DrawQuad(position, size, tint);
}

void Renderer2D::DrawSprite(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<TextureAsset>& texture,
                            const glm::vec4& tint)
{
    DrawSprite({position.x, position.y, 0.0f}, size, texture, tint);
}

void Renderer2D::DrawSprite(const glm::vec2& position, const glm::vec2& size, float rotation,
                            const std::shared_ptr<TextureAsset>& texture, const glm::vec4& tint)
{
    DrawSprite({position.x, position.y, 0.0f}, size, rotation, texture, tint);
}

void Renderer2D::DrawSprite(const glm::vec3& position, const glm::vec2& size, float rotation,
                            const std::shared_ptr<TextureAsset>& texture, const glm::vec4& tint)
{
    if (!texture || !texture->IsReady())
    {
        DrawQuad(position, size, rotation, tint);
        return;
    }

    uint32_t texId = texture->GetTexture().id;
    if (m_Data->QuadIndexCount > 0 && (m_Data->TextureSlotIndex == 0 || m_Data->TextureSlots[0] != texId))
    {
        NextBatch();
    }

    if (m_Data->QuadIndexCount == 0)
    {
        m_Data->TextureSlots[0] = texId;
        m_Data->TextureSlotIndex = 1;
    }

    DrawQuad(position, size, rotation, tint);
}

void Renderer2D::DrawString(const std::string& text, const glm::vec2& position, const std::shared_ptr<FontAsset>& font, float scale, const glm::vec4& color)
{
    DrawString(text, {position.x, position.y, 0.0f}, font, scale, color);
}

void Renderer2D::DrawString(const std::string& text, const glm::vec3& position, const std::shared_ptr<FontAsset>& font, float scale, const glm::vec4& color)
{
    if (!font || !font->IsReady()) return;

    const auto& nativeFont = font->GetFont();
    uint32_t texId = nativeFont.textureId;

    if (m_Data->QuadIndexCount > 0 && (m_Data->TextureSlotIndex == 0 || m_Data->TextureSlots[0] != texId))
    {
        NextBatch();
    }

    if (m_Data->QuadIndexCount == 0)
    {
        m_Data->TextureSlots[0] = texId;
        m_Data->TextureSlotIndex = 1;
    }

    float x = position.x;
    float y = position.y;

    for (char c : text)
    {
        if (c < 32 || c > 127) continue;

        const auto& q = nativeFont.chars[c - 32];
        
        float x_pos = x + q.xoff * scale;
        float y_pos = y + q.yoff * scale;
        float w = (q.x1 - q.x0) * nativeFont.atlasWidth * scale;
        float h = (q.y1 - q.y0) * nativeFont.atlasHeight * scale;

        if (m_Data->QuadIndexCount >= Renderer2DData::MaxIndices) NextBatch();

        // Manual quad submission for text (using char UVs)
        m_Data->QuadVertexBufferBase[m_Data->QuadIndexCount / 6 * 4 + 0] = { {x_pos, y_pos, position.z}, color, {q.x0, q.y0}, 0.0f };
        m_Data->QuadVertexBufferBase[m_Data->QuadIndexCount / 6 * 4 + 1] = { {x_pos + w, y_pos, position.z}, color, {q.x1, q.y0}, 0.0f };
        m_Data->QuadVertexBufferBase[m_Data->QuadIndexCount / 6 * 4 + 2] = { {x_pos + w, y_pos + h, position.z}, color, {q.x1, q.y1}, 0.0f };
        m_Data->QuadVertexBufferBase[m_Data->QuadIndexCount / 6 * 4 + 3] = { {x_pos, y_pos + h, position.z}, color, {q.x0, q.y1}, 0.0f };
        
        // Correcting the pointer increment which is handled differently in our batcher
        // Wait, DrawQuad has the logic. Let's redirect to DrawQuad but with UVs.
        // For simplicity, since DrawQuad doesn't take UVs, I'll just manually copy the logic from DrawQuad.
        
        m_Data->QuadVertexBufferPtr->Position = {x_pos, y_pos, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {q.x0, q.y0};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {x_pos + w, y_pos, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {q.x1, q.y0};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {x_pos + w, y_pos + h, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {q.x1, q.y1};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadVertexBufferPtr->Position = {x_pos, y_pos + h, position.z};
        m_Data->QuadVertexBufferPtr->Color = color;
        m_Data->QuadVertexBufferPtr->TexCoord = {q.x0, q.y1};
        m_Data->QuadVertexBufferPtr->TexIndex = 0;
        m_Data->QuadVertexBufferPtr++;

        m_Data->QuadIndexCount += 6;
        m_Data->Stats.QuadCount++;

        x += q.xadvance * scale;
    }
}

void Renderer2D::ResetStats()
{
    memset(&m_Data->Stats, 0, sizeof(Renderer2DData::Statistics));
}
} // namespace CHEngine
