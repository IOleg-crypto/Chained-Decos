#ifndef CH_FRAMEBUFFER_H
#define CH_FRAMEBUFFER_H

#include <memory>
#include "engine/graphics/api/texture.h"


namespace Chained
{
enum class FramebufferColorFormat
{
    RGBA8 = 0,
    RGBA16F = 1,
};

struct FramebufferSpecification
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t Samples = 1;
    bool SwapChainTarget = false;
    bool DepthOnly = false;  // When true: creates a pure depth texture (no color attachment). Used for shadow maps.
    FramebufferColorFormat ColorFormat = FramebufferColorFormat::RGBA8;
};

class Framebuffer
{
public:
    virtual ~Framebuffer() = default;

    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // No-op when Specification.Samples <= 1. Otherwise blits the multisample color/depth
    // attachments into an internal single-sample resolve target, which is what
    // GetColorAttachmentRendererID()/GetDepthAttachmentRendererID() return - multisample
    // textures can't be sampled with a plain sampler2D, so this must run once per frame
    // after rendering into the framebuffer and before it's read as a texture elsewhere
    // (e.g. by a composite/post-process pass or ImGui::Image).
    virtual void Resolve() = 0;

    virtual std::shared_ptr<Texture> GetColorAttachment() const = 0;
    virtual uint32_t GetColorAttachmentRendererID() const = 0;
    virtual std::shared_ptr<Texture> GetDepthAttachment() const = 0;
    virtual uint32_t GetDepthAttachmentRendererID() const = 0;

    virtual const FramebufferSpecification& GetSpecification() const = 0;

    virtual void* GetNativeFramebuffer() = 0;

    virtual bool IsValid() const = 0;

    static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
};

} // namespace Chained

#endif // CH_FRAMEBUFFER_H
