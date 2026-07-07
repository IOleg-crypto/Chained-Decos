#include "shader.h"
#include "renderer_api.h"
#include "engine/graphics/api/opengl/opengl_shader.h"
#include "engine/common/engine_assert.h"

namespace Chained {

std::shared_ptr<Shader> Shader::Create(const std::string& vsSource, const std::string& fsSource)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    CH_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLShader>(vsSource, fsSource);
    }
    CH_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace Chained
