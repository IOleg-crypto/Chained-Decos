#include "shader.h"
#include "engine/core/log.h"
#include "engine/core/types.h"
#include "opengl_shader.h"
#include "render_api.h"

namespace CHEngine
{
std::shared_ptr<Shader> Shader::Create(const std::string &filepath)
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLShader>(filepath);
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

std::shared_ptr<Shader> Shader::Create(const std::string &name, const std::string &vertexSrc,
                                       const std::string &fragmentSrc)
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

void ShaderLibrary::Add(const std::string &name, const std::shared_ptr<Shader> &shader)
{
    CH_CORE_ASSERT(!Exists(name), "Shader already exists!");
    m_Shaders[name] = shader;
}

void ShaderLibrary::Add(const std::shared_ptr<Shader> &shader)
{
    auto &name = shader->GetName();
    Add(name, shader);
}

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string &filepath)
{
    auto shader = Shader::Create(filepath);
    Add(shader);
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string &name, const std::string &filepath)
{
    auto shader = Shader::Create(filepath);
    Add(name, shader);
    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string &name)
{
    CH_CORE_ASSERT(Exists(name), "Shader not found!");
    return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string &name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}

} // namespace CHEngine
