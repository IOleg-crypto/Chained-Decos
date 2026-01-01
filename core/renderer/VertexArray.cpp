#include "VertexArray.h"
#include "OpenGLVertexArray.h"
#include "RendererAPI.h"
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
