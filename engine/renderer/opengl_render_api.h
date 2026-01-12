#ifndef CH_OPENGL_RENDERER_API_H
#define CH_OPENGL_RENDERER_API_H

#include "render_api.h"

namespace CHEngine
{
class OpenGLRenderAPI : public RenderAPI
{
public:
    virtual void Init() override;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    virtual void SetClearColor(const Color &color) override;
    virtual void Clear() override;

    virtual void DrawIndexed(uint32_t indexCount) override;
};
} // namespace CHEngine

#endif // CH_OPENGL_RENDERER_API_H
