#ifndef CH_SHADER_LIBRARY_H
#define CH_SHADER_LIBRARY_H

#include "engine/assets/types/shader_asset.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{

// Named shader cache with load-on-demand and hot-reload support.
class ShaderLibrary
{
public:
    ShaderLibrary();
    ~ShaderLibrary() = default;

    // Adds a shader under a unique library name; asserts if the name already exists.
    void Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader);
    // Adds a shader using the source file stem as the library key.
    void Add(const std::shared_ptr<ShaderAsset>& shader);

    // Loads a shader from disk and stores it under the source file stem.
    void Load(const std::string& path);
    // Loads a shader from disk and stores it under the provided name, replacing any existing entry.
    void Load(const std::string& name, const std::string& path);
    // Returns the cached shader if present; otherwise loads it from disk and caches it under the provided name.
    std::shared_ptr<ShaderAsset> LoadOrGet(const std::string& name, const std::string& path);

    // Returns the shader for a name; asserts if the name is missing.
    std::shared_ptr<ShaderAsset> Get(const std::string& name);
    // Returns the compiled GPU shader owned by the asset, or null if the asset has not produced one.
    std::shared_ptr<Shader> GetShader(const std::string& name);
    // Returns the shader asset matching a renderer ID by scanning the cache.
    std::shared_ptr<ShaderAsset> GetById(uint32_t id) const;
    // Checks whether a shader name exists in the cache.
    bool Exists(const std::string& name) const;

    // Exposes the cache for read-only iteration.
    const std::unordered_map<std::string, std::shared_ptr<ShaderAsset>>& GetShaders() const
    {
        return m_Shaders;
    }
    // Returns a snapshot of the current shader names.
    std::vector<std::string> GetNames() const;

    // Reloads every cached shader that still has a valid source path.
    void ReloadAll();

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderAsset>> m_Shaders;
};
} // namespace Chained

#endif // CH_SHADER_LIBRARY_H
