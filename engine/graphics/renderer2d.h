#ifndef CH_RENDERER2D_H
#define CH_RENDERER2D_H

#include "engine/core/base.h"
#include "engine/core/assert.h"
#include "raylib.h"
#include <memory>
#include <array>
#include <vector>

namespace CHEngine
{
    class TextureAsset;

    struct QuadVertex
    {
        Vector3 Position;
        Color Color;
        Vector2 TexCoord;
        float TexIndex;
    };

    struct Renderer2DData
    {
        static const uint32_t MaxQuads = 10000;
        static const uint32_t MaxVertices = MaxQuads * 4;
        static const uint32_t MaxIndices = MaxQuads * 6;
        static const uint32_t MaxTextureSlots = 32; // Limit by GPU

        QuadVertex* QuadVertexBufferBase = nullptr;
        QuadVertex* QuadVertexBufferPtr = nullptr;

        uint32_t QuadIndexCount = 0;

        std::array<Texture2D, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 = white texture (blank)

        // Stats
        struct Statistics
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;

            uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
            uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
        } Stats;
    };

    class Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        Renderer2D();
        ~Renderer2D();

    // Screen-space (UI) rendering
    void BeginCanvas();
    void EndCanvas();

    // World-space 2D rendering (Sprites, Billboards)
    void BeginScene(const Camera2D& camera);
    void EndScene();
    
    void Flush();

    // Primitives
    void DrawQuad(const Vector2& position, const Vector2& size, Color color);
    void DrawQuad(const Vector3& position, const Vector2& size, Color color);
    void DrawQuad(const Vector2& position, const Vector2& size, float rotation, Color color);
    void DrawQuad(const Vector3& position, const Vector2& size, float rotation, Color color);
    
    void DrawSprite(const Vector2& position, const Vector2& size, const std::shared_ptr<TextureAsset>& texture, Color tint = WHITE);
    void DrawSprite(const Vector3& position, const Vector2& size, const std::shared_ptr<TextureAsset>& texture, Color tint = WHITE);
    void DrawSprite(const Vector2& position, const Vector2& size, float rotation, const std::shared_ptr<TextureAsset>& texture, Color tint = WHITE);
    void DrawSprite(const Vector3& position, const Vector2& size, float rotation, const std::shared_ptr<TextureAsset>& texture, Color tint = WHITE);
    
    // Stats
    void ResetStats();
    Renderer2DData::Statistics GetStats() const { return m_Data->Stats; }

        static Renderer2D& Get() 
        { 
            CH_CORE_ASSERT(s_Instance, "Renderer2D instance is null!");
            return *s_Instance; 
        }

    private:
        void StartBatch();
        void NextBatch();

    private:
        static Renderer2D* s_Instance;
        std::unique_ptr<Renderer2DData> m_Data;
    };
}

#endif // CH_RENDERER2D_H
