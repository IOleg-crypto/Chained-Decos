#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/renderer_api.h"
#include "opengl/opengl_framebuffer.h"

namespace Chained
{
std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLFramebuffer>(spec);
        default : return nullptr;
    }
}

} // namespace Chained
