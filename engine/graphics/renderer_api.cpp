#include "renderer_api.h"
#include "raylib_renderer_api.h"

namespace CHEngine
{

RendererAPI::API RendererAPI::s_API = RendererAPI::API::Raylib;

std::unique_ptr<RendererAPI> RendererAPI::Create()
{
    switch (s_API)
    {
    case RendererAPI::API::None:
        return nullptr;
    case RendererAPI::API::Raylib:
        return std::make_unique<RaylibRendererAPI>();
    }

    return nullptr;
}

} // namespace CHEngine
