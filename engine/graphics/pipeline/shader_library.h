#ifndef CH_SHADER_LIBRARY_H
#define CH_SHADER_LIBRARY_H

#include "engine/graphics/assets/shader_asset.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
/// <summary>Named shader cache.</summary>
class ShaderLibrary
{
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;

    /// <summary>Adds a shader under a custom name.</summary>
    void Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader);
    /// <summary>Adds a shader using its own name.</summary>
    void Add(const std::shared_ptr<ShaderAsset>& shader);
    /// <summary>Loads a shader from disk.</summary>
    void Load(const std::string& path);
    /// <summary>Loads a shader and stores it under a custom name.</summary>
    void Load(const std::string& name, const std::string& path);

    /// <summary>Returns the shader for a name, if present.</summary>
    std::shared_ptr<ShaderAsset> Get(const std::string& name);
    /// <summary>Returns the compiled shader for a name, if present.</summary>
    std::shared_ptr<Shader> GetShader(const std::string& name);
    /// <summary>Returns the shader for an asset ID, if present.</summary>
    std::shared_ptr<ShaderAsset> GetById(uint32_t id) const;
    /// <summary>Checks whether a shader name exists.</summary>
    bool Exists(const std::string& name) const;

    /// <summary>Returns the backing shader map.</summary>
    const std::unordered_map<std::string, std::shared_ptr<ShaderAsset>>& GetShaders() const
    {
        return m_Shaders;
    }
    /// <summary>Returns all stored shader names.</summary>
    std::vector<std::string> GetNames() const;

    /// <summary>Reloads every stored shader.</summary>
    void ReloadAll();

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderAsset>> m_Shaders;
};
} // namespace CHEngine

#endif // CH_SHADER_LIBRARY_H
