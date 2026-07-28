#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/common/engine_assert.h"
#include "engine/core/log.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/project/project.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace Chained
{
ShaderStorage::ShaderStorage() = default;

void ShaderStorage::Insert(const std::string& name, const std::shared_ptr<ShaderAsset>& shader)
{
    m_Shaders[name] = shader;
    if (shader && shader->GetShader())
    {
        m_HandleToName[shader->GetShader()->GetNativeHandle()] = name;
    }
}

void ShaderStorage::Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader)
{
    CH_CORE_ASSERT(!Exists(name), "Shader already exists in library!");
    Insert(name, shader);
}

void ShaderStorage::Add(const std::shared_ptr<ShaderAsset>& shader)
{
    std::filesystem::path path = shader->GetPath();
    Add(path.stem().string(), shader);
}

void ShaderStorage::Load(const std::string& path)
{
    auto* am = ServiceLocator::TryGet<AssetManager>();
    auto shader = am ? am->Get<ShaderAsset>(path) : nullptr;
    if (shader)
    {
        Add(shader);
    }
}

void ShaderStorage::Load(const std::string& name, const std::string& path)
{
    auto* am = ServiceLocator::TryGet<AssetManager>();
    auto shader = am ? am->Get<ShaderAsset>(path) : nullptr;
    if (shader)
    {
        Insert(name, shader);
    }
}

std::shared_ptr<ShaderAsset> ShaderStorage::LoadOrGet(const std::string& name, const std::string& path)
{
    if (auto it = m_Shaders.find(name); it != m_Shaders.end())
    {
        return it->second;
    }

    auto* am = ServiceLocator::TryGet<AssetManager>();
    auto shader = am ? am->Get<ShaderAsset>(path) : nullptr;
    if (shader)
    {
        Insert(name, shader);
    }
    return shader;
}

std::shared_ptr<ShaderAsset> ShaderStorage::LoadOrGet(const std::string& name)
{
    if (auto it = m_Shaders.find(name); it != m_Shaders.end())
    {
        return it->second;
    }

    if (auto pathIt = m_ShaderPaths.find(name); pathIt != m_ShaderPaths.end())
    {
        return LoadOrGet(name, pathIt->second);
    }

    CH_CORE_ERROR("ShaderStorage: Could not find path for shader '{}' in configuration!", name);
    return nullptr;
}

void ShaderStorage::LoadConfig(const std::string& configPath)
{
    auto* am = ServiceLocator::TryGet<AssetManager>();
    std::string resolvedPath = am ? am->ResolvePath(configPath) : configPath;
    try
    {
        YAML::Node config = YAML::LoadFile(resolvedPath);
        if (config["Shaders"])
        {
            for (auto it = config["Shaders"].begin(); it != config["Shaders"].end(); ++it)
            {
                m_ShaderPaths[it->first.as<std::string>()] = it->second.as<std::string>();
            }
            CH_CORE_INFO("ShaderStorage: Loaded {} shader paths from config.", m_ShaderPaths.size());
        }
    }
    catch (const YAML::Exception& e)
    {
        CH_CORE_ERROR("ShaderStorage: Failed to load config file '{}': {}", configPath, e.what());
    }
}

std::shared_ptr<ShaderAsset> ShaderStorage::Get(const std::string& name)
{
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end())
    {
        return it->second;
    }
    CH_CORE_WARN("ShaderStorage: Shader '{}' not found!", name);
    return nullptr;
}

std::shared_ptr<Shader> ShaderStorage::GetShader(const std::string& name)
{
    if (auto asset = Get(name))
    {
        return asset->GetShader();
    }
    return nullptr;
}

std::shared_ptr<ShaderAsset> ShaderStorage::GetById(uint32_t id) const
{
    auto it = m_HandleToName.find(id);
    if (it != m_HandleToName.end())
    {
        auto shaderIt = m_Shaders.find(it->second);
        if (shaderIt != m_Shaders.end())
        {
            return shaderIt->second;
        }
    }
    return nullptr;
}

bool ShaderStorage::Exists(const std::string& name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}

std::vector<std::string> ShaderStorage::GetNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, shader] : m_Shaders)
    {
        names.push_back(name);
    }
    return names;
}

void ShaderStorage::ReloadAll()
{
    CH_CORE_INFO("ShaderStorage: Reloading all shaders...");

    for (auto& [name, shader] : m_Shaders)
    {
        if (shader && !shader->GetPath().empty())
        {
            CH_CORE_TRACE("ShaderStorage: Reloading shader '{}' from '{}'", name, shader->GetPath());
            if (auto* am = ServiceLocator::TryGet<AssetManager>())
                am->Reload<ShaderAsset>(shader->GetPath());
        }
    }

    // Re-fetch all from cache and update handle map
    for (auto& [name, shader] : m_Shaders)
    {
        if (shader && !shader->GetPath().empty())
        {
            if (auto* am = ServiceLocator::TryGet<AssetManager>())
                shader = am->Get<ShaderAsset>(shader->GetPath());
            Insert(name, shader);
        }
    }
}
void ShaderStorage::RebuildHandleMap()
{
    m_HandleToName.clear();
    for (const auto& [name, shader] : m_Shaders)
    {
        if (shader && shader->GetShader())
        {
            m_HandleToName[shader->GetShader()->GetNativeHandle()] = name;
        }
    }
}
} // namespace Chained