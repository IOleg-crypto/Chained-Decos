#include <filesystem>
#include <raylib.h>

#include "ShaderManager.h"
#include "core/Log.h"

// Alias for Raylib LoadShader function to avoid name conflict
namespace
{
auto LoadRaylibShader = ::LoadShader;
auto UnloadRaylibShader = ::UnloadShader;
} // namespace

ShaderManager::ShaderManager() : m_VertexShaderPath(""), m_FragmentShaderPath("")
{
}

ShaderManager::~ShaderManager()
{
    UnloadAllShaders();
}

bool ShaderManager::LoadVertexShader(const std::string &name, const std::string &vertexShaderPath)
{
    if (m_VertexShaderPath.empty())
    {
        return false;
    }
    if (m_shaders.find(name) != m_shaders.end())
    {
        CD_CORE_ERROR("ShaderManager::LoadVertexShader() - Shader '%s' already loaded",
                      name.c_str());
        return false;
    }
    if (!std::filesystem::exists(vertexShaderPath))
    {
        return false;
    }
    m_shaders[name] = std::make_shared<Shader>(
        LoadRaylibShader(vertexShaderPath.c_str(), m_FragmentShaderPath.c_str()));
    return true;
}
bool ShaderManager::LoadFragmentShader(const std::string &name,
                                       const std::string &fragmentShaderPath)
{
    if (m_FragmentShaderPath.empty())
    {
        return false;
    }
    if (m_shaders.find(name) != m_shaders.end())
    {
        CD_CORE_ERROR("ShaderManager::LoadFragmentShader() - Shader '%s' already loaded",
                      name.c_str());
        return false;
    }
    if (!std::filesystem::exists(fragmentShaderPath))
    {
        return false;
    }
    m_shaders[name] = std::make_shared<Shader>(
        LoadRaylibShader(m_VertexShaderPath.c_str(), fragmentShaderPath.c_str()));
    return true;
}

bool ShaderManager::LoadShaderPair(const std::string &name, const std::string &vertexShaderPath,
                                   const std::string &fragmentShaderPath)
{
    if (m_shaders.find(name) != m_shaders.end())
    {
        CD_CORE_INFO("ShaderManager::LoadShader() - Shader '%s' already loaded", name.c_str());
        return true;
    }

    if (!std::filesystem::exists(vertexShaderPath))
    {
        CD_CORE_ERROR("ShaderManager::LoadShader() - Vertex shader file not found: %s",
                      vertexShaderPath.c_str());
        return false;
    }

    if (!std::filesystem::exists(fragmentShaderPath))
    {
        CD_CORE_ERROR("ShaderManager::LoadShader() - Fragment shader file not found: %s",
                      fragmentShaderPath.c_str());
        return false;
    }

    Shader shader = LoadRaylibShader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    if (shader.id == 0)
    {
        CD_CORE_ERROR("ShaderManager::LoadShader() - Failed to load shader '%s' from %s + %s",
                      name.c_str(), vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        return false;
    }

    m_shaders[name] = std::make_shared<Shader>(shader);
    CD_CORE_INFO("ShaderManager::LoadShader() - Loaded shader '%s' from %s + %s", name.c_str(),
                 vertexShaderPath.c_str(), fragmentShaderPath.c_str());
    return true;
}
bool ShaderManager::UnloadVertexShader(const std::string &name)
{
    if (m_shaders.find(name) == m_shaders.end())
    {
        CD_CORE_ERROR("ShaderManager::UnloadVertexShader() - Shader '%s' not loaded", name.c_str());
        return false;
    }
    ::UnloadShader(*m_shaders[name]); // Use :: to call Raylib function
    m_shaders.erase(name);
    return true;
}
bool ShaderManager::UnloadFragmentShader(const std::string &name)
{
    if (m_shaders.find(name) == m_shaders.end())
    {
        CD_CORE_ERROR("ShaderManager::UnloadFragmentShader() - Shader '%s' not loaded",
                      name.c_str());
        return false;
    }
    ::UnloadShader(*m_shaders[name]); // Use :: to call Raylib function
    m_shaders.erase(name);
    return true;
}

bool ShaderManager::UnloadShader(const std::string &name)
{
    auto it = m_shaders.find(name);
    if (it == m_shaders.end())
    {
        CD_CORE_WARN("ShaderManager::UnloadShader() - Shader '%s' not loaded", name.c_str());
        return false;
    }
    ::UnloadShader(*it->second); // Use :: to call Raylib function
    m_shaders.erase(it);
    CD_CORE_INFO("ShaderManager::UnloadShader() - Unloaded shader '%s'", name.c_str());
    return true;
}
bool ShaderManager::UnloadAllShaders()
{
    for (auto &shader : m_shaders)
    {
        ::UnloadShader(*shader.second); // Use :: to call Raylib function
    }
    m_shaders.clear();
    return true;
}
bool ShaderManager::IsVertexShaderLoaded(const std::string &name) const
{
    return m_shaders.find(name) != m_shaders.end();
}
bool ShaderManager::IsFragmentShaderLoaded(const std::string &name) const
{
    return m_shaders.find(name) != m_shaders.end();
}
Shader *ShaderManager::GetVertexShader(const std::string &name)
{
    return m_shaders[name].get();
}
Shader *ShaderManager::GetFragmentShader(const std::string &name)
{
    return m_shaders[name].get();
}

bool ShaderManager::IsShaderLoaded(const std::string &name) const
{
    return m_shaders.find(name) != m_shaders.end();
}

Shader *ShaderManager::GetShader(const std::string &name)
{
    auto it = m_shaders.find(name);
    return (it != m_shaders.end()) ? it->second.get() : nullptr;
}
