#ifndef CH_RAYLIB_FRAMEBUFFER_H
#define CH_RAYLIB_FRAMEBUFFER_H

#include "framebuffer.h"
#include "raylib.h"

namespace CHEngine
{
class RaylibFramebuffer : public Framebuffer
{
public:
    RaylibFramebuffer(const FramebufferSpecification& spec);
    virtual ~RaylibFramebuffer();

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual void Resize(uint32_t width, uint32_t height) override;

    virtual uint32_t GetColorAttachmentRendererID() const override { return m_RenderTexture.texture.id; }

    virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    virtual void* GetNativeFramebuffer() override { return &m_RenderTexture; }

private:
    void Invalidate();

private:
    FramebufferSpecification m_Specification;
    RenderTexture2D m_RenderTexture = {0};
};

} // namespace CHEngine

#endif // CH_RAYLIB_FRAMEBUFFER_H
