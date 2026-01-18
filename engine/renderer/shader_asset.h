#ifndef CH_SHADER_ASSET_H
#define CH_SHADER_ASSET_H

#include "engine/core/base.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{
class ShaderAsset
{
public:
    ShaderAsset(Shader shader);
    ~ShaderAsset();

    static Ref<ShaderAsset> Load(const std::string &vsPath, const std::string &fsPath);

    Shader &GetShader()
    {
        return m_Shader;
    }
    const Shader &GetShader() const
    {
        return m_Shader;
    }

    int GetLocation(const std::string &name) const;
    void SetUniform(int loc, const void *value, int type);

private:
    Shader m_Shader;
};
} // namespace CHEngine

#endif // CH_SHADER_ASSET_H
