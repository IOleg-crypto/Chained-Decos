#ifndef CH_RENDERER2D_H
#define CH_RENDERER2D_H

#include "engine/core/ch_assert.h"
#include "engine/core/base.h"
#include <glm/glm.hpp>
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include <array>
#include <memory>
#include <vector>

#include "engine/graphics/api/camera_types.h"

namespace CHEngine
{
class TextureAsset;

struct QuadVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    float TexIndex;
};

struct Renderer2DData
{
    static const uint32_t MaxQuads = 10000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;
    static const uint32_t MaxTextureSlots = 32; // Limit by GPU

    std::shared_ptr<VertexArray> QuadVertexArray;
    std::shared_ptr<VertexBuffer> QuadVertexBuffer;

    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr = nullptr;

    uint32_t QuadIndexCount = 0;

    std::array<uint32_t, MaxTextureSlots> TextureSlots;
    uint32_t TextureSlotIndex = 1; // 0 = white texture (blank)

    glm::mat4 ViewProjection;

    // Stats
    struct Statistics
    {
        uint32_t DrawCalls = 0;
        uint32_t QuadCount = 0;

        uint32_t GetTotalVertexCount() const
        {
            return QuadCount * 4;
        }
        uint32_t GetTotalIndexCount() const
        {
            return QuadCount * 6;
        }
    } Stats;
};

class Renderer2D
{
public:
    Renderer2D();
    ~Renderer2D();

    static void Init();
    static void Shutdown();

    bool IsInitialized()
    {
        return m_Data != nullptr;
    }

    // World-space 2D rendering (Sprites, Billboards)
    void BeginScene(const Camera2D& camera);
    void EndScene();

    // Aliases for UI/Canvas rendering
    void BeginCanvas() { BeginScene(Camera2D()); }
    void EndCanvas() { EndScene(); }

    void Flush();

    // Primitives
    void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
    void DrawQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
    void DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);

    void DrawSprite(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<TextureAsset>& texture,
                    const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    void DrawSprite(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<TextureAsset>& texture,
                    const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    void DrawSprite(const glm::vec2& position, const glm::vec2& size, float rotation,
                    const std::shared_ptr<TextureAsset>& texture, const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    void DrawSprite(const glm::vec3& position, const glm::vec2& size, float rotation,
                    const std::shared_ptr<TextureAsset>& texture, const glm::vec4& tint = {1.0f, 1.0f, 1.0f, 1.0f});

    // Text Rendering
    void DrawString(const std::string& text, const glm::vec2& position, const std::shared_ptr<class FontAsset>& font, float scale = 1.0f, const glm::vec4& color = {1,1,1,1});
    void DrawString(const std::string& text, const glm::vec3& position, const std::shared_ptr<class FontAsset>& font, float scale = 1.0f, const glm::vec4& color = {1,1,1,1});

    // Stats
    void ResetStats();
    Renderer2DData::Statistics GetStats() const
    {
        return m_Data->Stats;
    }

    static Renderer2D& Get();

private:
    void StartBatch();
    void NextBatch();
    void BeginMode2D(const Camera2D& camera);

private:
    static Renderer2D* s_Instance;
    std::unique_ptr<Renderer2DData> m_Data;
};
} // namespace CHEngine

#endif // CH_RENDERER2D_H
