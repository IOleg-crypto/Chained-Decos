#ifndef CH_RENDER_COMMAND_H
#define CH_RENDER_COMMAND_H

#include "engine/graphics/api/renderer_api.h"

namespace Chained
{
// Thin static wrapper over the active RendererAPI.
class RenderCommand
{
public:
    static void Initialize();
    static void Shutdown();

    static void Clear(Color color)
    {
        s_RendererAPI->SetClearColor(color);
        s_RendererAPI->Clear();
    }
    static void SetViewport(int x, int y, int width, int height)
    {
        s_RendererAPI->SetViewport(x, y, width, height);
    }
 
    static void SetDepthFunc(RendererAPI::DepthFunc func)
    {
        s_RendererAPI->SetDepthFunc(func);
    }

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0)
    {
        s_RendererAPI->DrawIndexed(vertexArray, indexCount);
    }
    
    static void DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray, uint32_t instanceCount, uint32_t indexCount = 0)
    {
        s_RendererAPI->DrawIndexedInstanced(vertexArray, instanceCount, indexCount);
    }

    static void DrawIndexedLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0)
    {
        s_RendererAPI->DrawIndexedLines(vertexArray, indexCount);
    }

    static void DrawArrays(uint32_t vertexCount)
    {
        s_RendererAPI->DrawArrays(vertexCount);
    }

    static void DrawArraysInstanced(uint32_t vertexCount, uint32_t instanceCount)
    {
        s_RendererAPI->DrawArraysInstanced(vertexCount, instanceCount);
    }

    static void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
    {
        s_RendererAPI->DrawLines(vertexArray, vertexCount);
    }

    static void SetTexture(uint32_t slot, uint32_t textureId, bool isCubemap = false)
    {
        s_RendererAPI->SetTexture(slot, textureId, isCubemap);
    }

    // Removed empty DrawLine and DrawGrid stubs. Use Renderer::DrawLine/DrawGrid instead.
    static void SetCullMode(RendererAPI::CullMode mode)
    {
        s_RendererAPI->SetCullMode(mode);
    }

    static void SetBlendFunc(RendererAPI::BlendFactor src, RendererAPI::BlendFactor dst)
    {
        s_RendererAPI->SetBlendFunc(src, dst);
    }

    static void SetLineWidth(float width)
    {
        s_RendererAPI->SetLineWidth(width);
    }

    static void EnableDepthTest() { s_RendererAPI->SetDepthTest(true); }
    static void DisableDepthTest() { s_RendererAPI->SetDepthTest(false); }
    static void SetDepthTest(bool enabled) { s_RendererAPI->SetDepthTest(enabled); }
    static void EnableDepthMask() { s_RendererAPI->SetDepthMask(true); }
    static void DisableDepthMask() { s_RendererAPI->SetDepthMask(false); }
    static void SetDepthMask(bool enabled) { s_RendererAPI->SetDepthMask(enabled); }
    static void SetBlendMode(bool enabled) { s_RendererAPI->SetBlendMode(enabled); }

    static void SetPolygonMode(RendererAPI::PolygonMode mode) { s_RendererAPI->SetPolygonMode(mode); }
    static void SetPolygonOffset(bool enabled, float factor = 0.0f, float units = 0.0f) { s_RendererAPI->SetPolygonOffset(enabled, factor, units); }

    static bool IsDepthTestEnabled() { return s_RendererAPI->IsDepthTestEnabled(); }
    static bool IsBlendEnabled() { return s_RendererAPI->IsBlendEnabled(); }
    static bool IsCullFaceEnabled() { return s_RendererAPI->IsCullFaceEnabled(); }

private:
    static std::unique_ptr<RendererAPI> s_RendererAPI;
};
} // namespace Chained

#endif // CH_RENDER_COMMAND_H
