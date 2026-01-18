#include "shader_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"

namespace CHEngine
{
ShaderAsset::ShaderAsset(Shader shader) : m_Shader(shader)
{
}

ShaderAsset::~ShaderAsset()
{
    if (m_Shader.id > 0)
    {
        UnloadShader(m_Shader);
    }
}

Ref<ShaderAsset> ShaderAsset::Load(const std::string &vsPath, const std::string &fsPath)
{
    auto vsAbs = Assets::ResolvePath(vsPath);
    auto fsAbs = Assets::ResolvePath(fsPath);

    Shader shader = ::LoadShader(vsAbs.string().c_str(), fsAbs.string().c_str());
    if (shader.id > 0)
    {
        return CreateRef<ShaderAsset>(shader);
    }

    CH_CORE_ERROR("Failed to load shader: VS: {}, FS: {}", vsPath, fsPath);
    return nullptr;
}

int ShaderAsset::GetLocation(const std::string &name) const
{
    return GetShaderLocation(m_Shader, name.c_str());
}

void ShaderAsset::SetUniform(int loc, const void *value, int type)
{
    SetShaderValue(m_Shader, loc, value, type);
}
} // namespace CHEngine
