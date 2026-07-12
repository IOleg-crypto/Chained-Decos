#ifndef CH_FRAMEBUFFER_H
#define CH_FRAMEBUFFER_H

#include <memory>


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

    virtual uint32_t GetColorAttachmentRendererID() const = 0;
    virtual uint32_t GetDepthAttachmentRendererID() const = 0;

    virtual const FramebufferSpecification& GetSpecification() const = 0;

    virtual void* GetNativeFramebuffer() = 0;

    virtual bool IsValid() const = 0;

    static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
};

} // namespace Chained

#endif // CH_FRAMEBUFFER_H
