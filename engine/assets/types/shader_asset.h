#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/shader.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <memory>
#include <vector>

namespace Chained
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
    virtual ~ShaderAsset() override = default;

    static AssetType GetStaticType()
    {
        return AssetType::Shader;
    }

    size_t GetMemoryUsage() const override
    {
        // Shader sizing on RAM is minuscule compared to GPU
        return sizeof(*this);
    }

    std::shared_ptr<Shader> GetShader() const
    {
        return m_Shader;
    }
    void SetShader(const std::shared_ptr<Shader>& shader)
    {
        m_Shader = shader;
    }

    const std::vector<ShaderUniform>& GetUniforms() const
    {
        return m_Uniforms;
    }
    void SetUniforms(const std::vector<ShaderUniform>& uniforms)
    {
        m_Uniforms = uniforms;
    }

private:
    std::shared_ptr<Shader> m_Shader;
    std::vector<ShaderUniform> m_Uniforms;
};
} // namespace Chained

#endif // CH_SHADER_ASSET_H