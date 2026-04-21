#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "engine/core/assets/asset.h"
#include "engine/graphics/api/shader.h"
#include <memory>

namespace CHEngine
{

class ShaderAsset : public Asset
{
public:
    ShaderAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~ShaderAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Shader;
    }

    void OnLoaded() override {} // Shaders are usually loaded synchronously on GPU

    std::shared_ptr<Shader> GetShader() const { return m_Shader; }
    void SetShader(const std::shared_ptr<Shader>& shader) { m_Shader = shader; }

    const std::vector<std::string>& GetUniformNames() const { return m_UniformNames; }
    void SetUniformNames(const std::vector<std::string>& names) { m_UniformNames = names; }

private:
    std::shared_ptr<Shader> m_Shader;
    std::vector<std::string> m_UniformNames;
};

} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
