#include "engine/graphics/pipeline/shader_storage.h"
#include "engine/foundation/engine_assert.h"
#include "engine/core/log.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/project/project.h"
#include "service_locator.h"
#include <filesystem>

namespace Chained
{
ShaderStorage::ShaderStorage()
{
}

void ShaderStorage::Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader)
{
    CH_CORE_ASSERT(!Exists(name), "Shader already exists in library!");
    m_Shaders[name] = shader;
}

void ShaderStorage::Add(const std::shared_ptr<ShaderAsset>& shader)
{
    std::filesystem::path path = shader->GetPath();
    Add(path.stem().string(), shader);
}

void ShaderStorage::Load(const std::string& path)
{
    auto shader = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(path);
    if (shader)
    {
        Add(shader);
    }
}

void ShaderStorage::Load(const std::string& name, const std::string& path)
{
    auto shader = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(path);
    if (shader)
    {
        if (Exists(name))
        {
            m_Shaders[name] = shader;
        }
        else
        {
            Add(name, shader);
        }
    }
}

std::shared_ptr<ShaderAsset> ShaderStorage::LoadOrGet(const std::string& name, const std::string& path)
{
    if (auto it = m_Shaders.find(name); it != m_Shaders.end())
    {
        return it->second;
    }

    auto shader = ServiceLocator::Get<AssetManager>()->Get<ShaderAsset>(path);
    if (shader)
    {
        m_Shaders[name] = shader;
    }
    return shader;
}

std::shared_ptr<ShaderAsset> ShaderStorage::Get(const std::string& name)
{
    CH_CORE_ASSERT(Exists(name), "Shader name not found!");
    return m_Shaders[name];
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
    for (const auto& [name, shader] : m_Shaders)
    {
        if (shader->GetShader() && shader->GetShader()->GetRendererID() == id)
            return shader;
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
            ServiceLocator::Get<AssetManager>()->Reload<ShaderAsset>(shader->GetPath());
        }
    }
}
} // namespace CHEngine