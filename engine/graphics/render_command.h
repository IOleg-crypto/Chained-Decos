#ifndef CH_RENDER_COMMAND_H
#define CH_RENDER_COMMAND_H

#include "renderer_api.h"

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

    static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0)
    {
        s_RendererAPI->DrawIndexed(vertexArray, indexCount);
    }

    static void DrawLine(Vector3 startPosition, Vector3 endPosition, Color color);
    static void DrawGrid(int sliceCount, float spacing);

    static void SetBlendMode(int blendMode);
    static void EnableDepthTest();
    static void DisableDepthTest();
    static void EnableBackfaceCulling();
    static void DisableBackfaceCulling();
    static void EnableDepthMask();
    static void DisableDepthMask();

private:
    static std::unique_ptr<RendererAPI> s_RendererAPI;
};
} // namespace CHEngine

#endif // CH_RENDER_COMMAND_H
