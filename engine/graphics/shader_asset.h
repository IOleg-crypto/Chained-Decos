#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "asset.h"
#include "engine/core/base.h"
#include "raylib.h"
#include "string"
#include "unordered_map"

namespace CHEngine
{
class ShaderAsset : public Asset
{
public:
    ShaderAsset() = default;
    ShaderAsset(Shader shader);
    virtual ~ShaderAsset();

    void UploadToGPU();
    void LoadFromFile(const std::string &path);

    static std::shared_ptr<ShaderAsset> Load(const std::string &vsPath, const std::string &fsPath);
    static std::shared_ptr<ShaderAsset> Load(const std::string &chshaderPath);

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
    Shader m_Shader = { 0 };
    std::unordered_map<std::string, int> m_UniformCache;
};
} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
