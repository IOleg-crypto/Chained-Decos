#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/api/graphics_device.h"
#include "opengl/gl_vertex_array.h"

namespace Chained
{

std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (GraphicsDevice::GetAPI())
    {
        case GraphicsDevice::API::None:    return nullptr;
        case GraphicsDevice::API::OpenGL:  return std::make_shared<GLVertexArray>();
        default : return nullptr;
    }
}

} // namespace Chained
