#include "shader.h"
#include "graphics_device.h"
#include "engine/graphics/api/opengl/gl_shader_program.h"
#include "engine/common/engine_assert.h"

namespace Chained
{

std::shared_ptr<Shader> Shader::Create(const std::string& vsSource, const std::string& fsSource)
{
    switch (GraphicsDevice::GetAPI())
    {
    case GraphicsDevice::API::None:
        CH_CORE_ASSERT(false, "GraphicsDevice::None is currently not supported!");
        return nullptr;
    case GraphicsDevice::API::OpenGL:
        return std::make_shared<GLShaderProgram>(vsSource, fsSource);
    }
    CH_CORE_ASSERT(false, "Unknown GraphicsDevice!");
    return nullptr;
}

} // namespace Chained
