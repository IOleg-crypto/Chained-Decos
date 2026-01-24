#pragma once
#include "asset.h"
#include "engine/core/base.h"
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class ShaderAsset : public Asset
{
public:
    ShaderAsset(Shader shader);
    virtual ~ShaderAsset();

    static Ref<ShaderAsset> Load(const std::string &vsPath, const std::string &fsPath);
    static Ref<ShaderAsset> Load(const std::string &chshaderPath);

    virtual AssetType GetType() const override
    {
        return AssetType::Shader;
    }

    Shader &GetShader()
    {
        return m_Shader;
    }
    const Shader &GetShader() const
    {
        return m_Shader;
    }

    int GetLocation(const std::string &name);
    void SetUniform(int loc, const void *value, int type);
    void SetUniform(const std::string &name, const void *value, int type);

private:
    Shader m_Shader;
    std::unordered_map<std::string, int> m_UniformCache;
};
} // namespace CHEngine
