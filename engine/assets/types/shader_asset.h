#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/shader.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <memory>
#include <vector>

namespace CHEngine
{

class ShaderAsset : public Asset
{
public:
    ShaderAsset()
        : Asset(GetStaticType())
    {
    }
    ShaderAsset(AssetHandle handle)
        : Asset(GetStaticType(), handle)
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

    const std::vector<ShaderUniform>& GetUniforms() const { return m_Uniforms; }
    void SetUniforms(const std::vector<ShaderUniform>& uniforms) { m_Uniforms = uniforms; }

private:
    std::shared_ptr<Shader> m_Shader;
    std::vector<ShaderUniform> m_Uniforms;
};

} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
