#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/graphics_device.h"
#include "opengl/gl_framebuffer.h"

namespace Chained
{
std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
    switch (GraphicsDevice::GetAPI())
    {
    case GraphicsDevice::API::None:
        return nullptr;
    case GraphicsDevice::API::OpenGL:
        return std::make_shared<GLFramebuffer>(spec);
    default:
        return nullptr;
    }
}

} // namespace Chained
