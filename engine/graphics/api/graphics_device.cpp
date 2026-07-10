#include "engine/graphics/api/graphics_device.h"
#include "opengl/gl_device.h"

namespace Chained
{

GraphicsDevice* GraphicsDevice::s_Instance = nullptr;
GraphicsDevice::API GraphicsDevice::s_API = GraphicsDevice::API::OpenGL;

std::unique_ptr<GraphicsDevice> GraphicsDevice::Create()
{
    switch (s_API)
    {
    case GraphicsDevice::API::None:
        return nullptr;
    case GraphicsDevice::API::OpenGL:
        return std::make_unique<GLDevice>();
    default:
        return nullptr;
    }
}

} // namespace Chained
