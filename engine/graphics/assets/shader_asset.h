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

private:
    std::shared_ptr<Shader> m_Shader;
};

} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
