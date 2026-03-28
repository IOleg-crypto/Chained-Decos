#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/api/renderer_api.h"
#include "opengl/opengl_vertex_array.h"

namespace CHEngine
{

std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexArray>();
    }
    return nullptr;
}

} // namespace CHEngine
