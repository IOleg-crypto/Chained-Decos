#ifndef CH_OPENGL_FRAMEBUFFER_H
#define CH_OPENGL_FRAMEBUFFER_H

#include "engine/graphics/api/framebuffer.h"

namespace CHEngine
{
class OpenGLFramebuffer : public Framebuffer
{
public:
    OpenGLFramebuffer(const FramebufferSpecification& spec);
    virtual ~OpenGLFramebuffer();

    void Invalidate();

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual void Resize(uint32_t width, uint32_t height) override;

    virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
    virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

    virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    virtual void* GetNativeFramebuffer() override { return (void*)(uintptr_t)m_RendererID; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    FramebufferSpecification m_Specification;
};
}

#endif // CH_OPENGL_FRAMEBUFFER_H
