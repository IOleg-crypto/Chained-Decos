#include "core/log.h"
#include "vertex_array.h"
#include "opengl_vertex_array.h"
#include "renderer_api.h"
#include "core/Base.h"

namespace CHEngine
{

std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexArray>();
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace CHEngine
#include "core/log.h"

