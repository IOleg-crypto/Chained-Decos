#include "gl_framebuffer.h"
#include "engine/common/engine_assert.h"
#include <glad/gl.h>

namespace Chained
{
GLFramebuffer::GLFramebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec)
{
    Invalidate();
}

GLFramebuffer::~GLFramebuffer()
{
    glDeleteFramebuffers(1, &m_RendererID);
    glDeleteTextures(1, &m_ColorAttachment);
    glDeleteTextures(1, &m_DepthAttachment);
}

void GLFramebuffer::Invalidate()
{
    GLint previousFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

    if (m_RendererID)
    {
        glDeleteFramebuffers(1, &m_RendererID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteTextures(1, &m_DepthAttachment);
        m_RendererID = 0;
        m_ColorAttachment = 0;
        m_DepthAttachment = 0;
    }

    m_IsComplete = false;

    if (m_Specification.Width == 0 || m_Specification.Height == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        return;
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    if (m_Specification.DepthOnly)
    {
        // ── Depth-only path (shadow map) ────────────────────────────────
        // No color attachment; depth is written to a sampable 2D texture.
        glGenTextures(1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                     m_Specification.Width, m_Specification.Height,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // Clamp-to-border with depth=1 so fragments outside the light frustum are NOT in shadow.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);

        // No color buffer
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        // ── Standard color + depth path ─────────────────────────────────
        // Color Attachment
        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        GLenum internalFormat = GL_RGBA8;
        GLenum dataType = GL_UNSIGNED_BYTE;
        if (m_Specification.ColorFormat == FramebufferColorFormat::RGBA16F)
        {
            internalFormat = GL_RGBA16F;
            dataType = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Specification.Width, m_Specification.Height, 0, GL_RGBA,
                     dataType, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        // Depth Attachment
        glGenTextures(1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        CH_CORE_ERROR("Framebuffer is incomplete! Status: 0x{0:X}, Width: {1}, Height: {2}", status, m_Specification.Width, m_Specification.Height);
        // Do NOT assert here — a non-complete FBO should degrade gracefully, not crash the editor.
    }
    else
    {
        m_IsComplete = true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
}

void GLFramebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Specification.Width, m_Specification.Height);
}

void GLFramebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFramebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > 8192 || height > 8192)
    {
        return;
    }
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

}
