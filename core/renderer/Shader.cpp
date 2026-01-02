#include "core/log.h"
#include "shader.h"
#include "opengl_shader.h"
#include "renderer_api.h"
#include "core/Base.h"

namespace CHEngine
{

std::shared_ptr<Shader> Shader::Create(const std::string &filepath)
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLShader>(filepath);
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

std::shared_ptr<Shader> Shader::Create(const std::string &name, const std::string &vertexSrc,
                                       const std::string &fragmentSrc)
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLShader>(name, vertexSrc, fragmentSrc);
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

void ShaderLibrary::Add(const std::string &name, const std::shared_ptr<Shader> &shader)
{
    CD_CORE_ASSERT(!Exists(name), "Shader already exists!");
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
    CD_CORE_ASSERT(Exists(name), "Shader not found!");
    return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string &name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}

} // namespace CHEngine
#include "core/log.h"

