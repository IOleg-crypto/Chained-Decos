#ifndef CD_CORE_RENDERER_OPENGL_RENDERER_API_H
#define CD_CORE_RENDERER_OPENGL_RENDERER_API_H

#include "renderer_api.h"

namespace CHEngine
{

class OpenGLRendererAPI : public RendererAPI
{
public:
    virtual void Init() override;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    virtual void SetClearColor(const Vector4 &color) override;
    virtual void Clear() override;

    virtual void DrawIndexed(uint32_t indexCount) override;
};

} // namespace CHEngine

#endif // CD_CORE_RENDERER_OPENGL_RENDERER_API_H
