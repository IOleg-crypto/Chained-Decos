#include "raylib_framebuffer.h"
#include "engine/core/assert.h"

namespace CHEngine
{
RaylibFramebuffer::RaylibFramebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec)
{
    Invalidate();
}

RaylibFramebuffer::~RaylibFramebuffer()
{
    if (m_RenderTexture.id > 0)
    {
        UnloadRenderTexture(m_RenderTexture);
    }
}

void RaylibFramebuffer::Invalidate()
{
    if (m_RenderTexture.id > 0)
    {
        UnloadRenderTexture(m_RenderTexture);
    }

    m_RenderTexture = LoadRenderTexture(m_Specification.Width, m_Specification.Height);
}

void RaylibFramebuffer::Bind()
{
    BeginTextureMode(m_RenderTexture);
}

void RaylibFramebuffer::Unbind()
{
    EndTextureMode();
}

void RaylibFramebuffer::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

} // namespace CHEngine
