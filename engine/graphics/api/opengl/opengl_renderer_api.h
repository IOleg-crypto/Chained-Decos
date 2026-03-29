#ifndef CH_OPENGL_RENDERER_API_H
#define CH_OPENGL_RENDERER_API_H

#include "engine/graphics/api/renderer_api.h"

namespace CHEngine
{

class OpenGLRendererAPI : public RendererAPI
{
public:
    virtual void Init() override;
    virtual void SetViewport(int x, int y, int width, int height) override;
    virtual void SetClearColor(const Color& color) override;
    virtual void Clear() override;
    virtual void SetDepthFunc(DepthFunc func) override;
    virtual void SetDepthTest(bool enabled) override;
    virtual void SetDepthMask(bool enabled) override;
    
    virtual void SetCullMode(CullMode mode) override;
    virtual void SetBlendMode(bool enabled) override;
    virtual void SetBlendFunc(BlendFactor src, BlendFactor dst) override;

    virtual void SetLineWidth(float width) override;

    virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    virtual void DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount) override;

private:
    float m_ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
};

} // namespace CHEngine

#endif // CH_OPENGL_RENDERER_API_H
