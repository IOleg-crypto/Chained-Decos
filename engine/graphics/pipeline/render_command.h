#ifndef CH_RENDER_COMMAND_H
#define CH_RENDER_COMMAND_H

#include "engine/graphics/api/renderer_api.h"

namespace CHEngine
{
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
    static void EnableDepthMask() { s_RendererAPI->SetDepthMask(true); }
    static void DisableDepthMask() { s_RendererAPI->SetDepthMask(false); }
    static void SetBlendMode(bool enabled) { s_RendererAPI->SetBlendMode(enabled); }

private:
    static std::unique_ptr<RendererAPI> s_RendererAPI;
};
} // namespace CHEngine

#endif // CH_RENDER_COMMAND_H
