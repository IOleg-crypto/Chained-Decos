#ifndef CH_SHADER_LIBRARY_H
#define CH_SHADER_LIBRARY_H

#include "engine/core/base.h"
#include "engine/graphics/shader_asset.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class ShaderLibrary
{
public:
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;

    void Add(const std::string& name, const std::shared_ptr<ShaderAsset>& shader);
    void Load(const std::string& name, const std::string& path);

    std::shared_ptr<ShaderAsset> Get(const std::string& name);
    bool Exists(const std::string& name) const;

    const std::unordered_map<std::string, std::shared_ptr<ShaderAsset>>& GetShaders() const
    {
        return m_Shaders;
    }
    std::vector<std::string> GetNames() const;

private:
    std::unordered_map<std::string, std::shared_ptr<ShaderAsset>> m_Shaders;
};
} // namespace CHEngine

#endif // CH_SHADER_LIBRARY_H
