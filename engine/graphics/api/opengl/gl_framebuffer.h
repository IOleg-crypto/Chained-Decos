#ifndef CH_OPENGL_FRAMEBUFFER_H
#define CH_OPENGL_FRAMEBUFFER_H

#include "engine/graphics/api/framebuffer.h"

namespace Chained
{
class GLFramebuffer : public Framebuffer
{
public:
    GLFramebuffer(const FramebufferSpecification& spec);
    virtual ~GLFramebuffer();

    void Invalidate();

    virtual void Bind() override;
    virtual void Unbind() override;

    virtual void Resize(uint32_t width, uint32_t height) override;

    virtual uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
    virtual uint32_t GetDepthAttachmentRendererID() const override { return m_DepthAttachment; }

    virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }

    virtual void* GetNativeFramebuffer() override { return (void*)(uintptr_t)m_RendererID; }

    virtual bool IsValid() const override { return m_RendererID != 0 && m_IsComplete; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    FramebufferSpecification m_Specification;
    bool m_IsComplete = false;
};
}

#endif // CH_OPENGL_FRAMEBUFFER_H
