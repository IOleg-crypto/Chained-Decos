#include "vertex_array.h"
#include "engine/core/log.h"
#include "engine/core/types.h"
#include "opengl_vertex_array.h"
#include "render_api.h"

namespace CHEngine
{
std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexArray>();
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

} // namespace CHEngine
