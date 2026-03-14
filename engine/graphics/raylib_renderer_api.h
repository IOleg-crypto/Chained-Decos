#ifndef CH_RAYLIB_RENDERER_API_H
#define CH_RAYLIB_RENDERER_API_H

#include "renderer_api.h"

namespace CHEngine
{

class RaylibRendererAPI : public RendererAPI
{
public:
    virtual void Init() override;
    virtual void SetViewport(int x, int y, int width, int height) override;
    virtual void SetClearColor(const Color& color) override;
    virtual void Clear() override;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
};

} // namespace CHEngine

#endif // CH_RAYLIB_RENDERER_API_H
