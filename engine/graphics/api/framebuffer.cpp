#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/renderer_api.h"
#include "raylib/raylib_framebuffer.h"

namespace CHEngine
{
std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::Raylib:  return std::make_shared<RaylibFramebuffer>(spec);
    }
    return nullptr;
}

} // namespace CHEngine
