#include "shader_library.h"
#include "engine/core/assert.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
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
    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto shader = project->GetAssetManager()->Get<ShaderAsset>(path);
    if (shader)
    {
        Add(name, shader);
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
} // namespace CHEngine
