#include "shader_library.h"
#include "engine/core/ch_assert.h"
#include "engine/core/log.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/scene/project.h"

namespace CHEngine
{
void ShaderLibrary::Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader)
{
    CH_CORE_ASSERT(!Exists(name), "Shader already exists in library!");
    m_Shaders[name] = shader;
}

void ShaderLibrary::Load(const std::string& name, const std::string& path)
{
    auto shader = AssetManager::Get().Get<ShaderAsset>(path);
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

std::shared_ptr<ShaderAsset> ShaderLibrary::Get(const std::string& name)
{
    CH_CORE_ASSERT(Exists(name), "Shader not found in library!");
    return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}

std::vector<std::string> ShaderLibrary::GetNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, shader] : m_Shaders)
    {
        names.push_back(name);
    }
    return names;
}

void ShaderLibrary::ReloadAll()
{
    CH_CORE_INFO("ShaderLibrary: Reloading all shaders...");
    
    // We need to collect paths first because reloading modifies the metadata in AssetManager
    std::vector<std::pair<std::string, std::string>> namePaths;
    for (const auto& [name, shader] : m_Shaders)
    {
        namePaths.push_back({ name, shader->GetPath() });
    }

    for (const auto& [name, path] : namePaths)
    {
        CH_CORE_TRACE("ShaderLibrary: Reloading shader '{}' from '{}'", name, path);
        // Reload in AssetManager
        AssetManager::Get().Reload<ShaderAsset>(path);
        // Re-get and update in our map
        m_Shaders[name] = AssetManager::Get().Get<ShaderAsset>(path);
    }
}
} // namespace CHEngine
