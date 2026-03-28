#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "engine/core/assets/asset.h"
#include "engine/core/base.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace CHEngine
{

struct NativeShader
{
    uint32_t id = 0;
    // Raylib Shader struct has locs array but we'll use a cache in ShaderAsset
};

class ShaderAsset : public Asset
{
public:
    static AssetType GetStaticType()
    {
        return AssetType::Shader;
    }

    ShaderAsset()
        : Asset(GetStaticType())
    {
    }
    ShaderAsset(const NativeShader& shader)
        : Asset(GetStaticType()),
          m_Shader(shader)
    {
    }
    virtual ~ShaderAsset();

    void UploadToGPU()
    {
    }

    void SetShader(const NativeShader& shader)
    {
        m_Shader = shader;
    }

    NativeShader& GetShader()
    {
        return m_Shader;
    }
    const NativeShader& GetShader() const
    {
        return m_Shader;
    }

    int GetLocation(const std::string& name);
    void SetUniform(int location, const void* value, int type);
    void SetUniform(const std::string& name, const void* value, int type);

    // Type-safe helper methods
    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetVec2(const std::string& name, const glm::vec2& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetColor(const std::string& name, const Color& value);
    void SetMatrix(const std::string& name, const glm::mat4& value);
    void SetMatrices(const std::string& name, const glm::mat4* values, int count);


private:
    NativeShader m_Shader = {0};
    std::unordered_map<std::string, int> m_UniformCache;
};
} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
