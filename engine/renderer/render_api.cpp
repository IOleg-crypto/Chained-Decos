#include "render_api.h"
#include "opengl_render_api.h"

namespace CHEngine
{
RenderAPI::API RenderAPI::s_API = RenderAPI::API::OpenGL;

std::unique_ptr<RenderAPI> RenderAPI::Create()
{
    switch (s_API)
    {
    case RenderAPI::API::None:
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_unique<OpenGLRenderAPI>();
        // In future we will add more APIs( Vulkan, DirectX, Metal, etc.)
    }

    return nullptr;
}

} // namespace CHEngine
