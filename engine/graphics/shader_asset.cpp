#include "shader_asset.h"
#include "engine/core/log.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>


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

std::shared_ptr<ShaderAsset> ShaderAsset::Load(const std::string &vsPath, const std::string &fsPath)
{
    std::filesystem::path vsAbs(vsPath);
    std::filesystem::path fsAbs(fsPath);

    Shader shader = ::LoadShader(vsAbs.string().c_str(), fsAbs.string().c_str());
    if (shader.id > 0)
    {
        auto asset = std::make_shared<ShaderAsset>(shader);
        asset->SetPath(vsPath + "|" + fsPath);
        return asset;
    }

    CH_CORE_ERROR("Failed to load shader: VS: {}, FS: {}", vsPath, fsPath);
    return nullptr;
}

std::shared_ptr<ShaderAsset> ShaderAsset::Load(const std::string &chshaderPath)
{
    std::filesystem::path absolutePath(chshaderPath);
    if (!std::filesystem::exists(absolutePath))
    {
        CH_CORE_ERROR("CHShader not found: {}", chshaderPath);
        return nullptr;
    }

    try
    {
        YAML::Node config = YAML::LoadFile(absolutePath.string());
        std::string vsRel = config["VertexShader"].as<std::string>();
        std::string fsRel = config["FragmentShader"].as<std::string>();

        std::shared_ptr<ShaderAsset> asset = Load(vsRel, fsRel);
        if (asset)
        {
            asset->SetPath(chshaderPath);

            // Automatic uniform caching and Raylib standard mapping
            if (config["Uniforms"])
            {
                for (auto uniform : config["Uniforms"])
                {
                    std::string name = uniform.as<std::string>();
                    int loc = asset->GetLocation(name);

                    // Map standard names to Raylib locs
                    if (name == "mvp")
                        asset->m_Shader.locs[SHADER_LOC_MATRIX_MVP] = loc;
                    else if (name == "matModel")
                        asset->m_Shader.locs[SHADER_LOC_MATRIX_MODEL] = loc;
                    else if (name == "matNormal")
                        asset->m_Shader.locs[SHADER_LOC_MATRIX_NORMAL] = loc;
                    else if (name == "matView")
                        asset->m_Shader.locs[SHADER_LOC_MATRIX_VIEW] = loc;
                    else if (name == "matProjection")
                        asset->m_Shader.locs[SHADER_LOC_MATRIX_PROJECTION] = loc;
                    else if (name == "viewPos")
                        asset->m_Shader.locs[SHADER_LOC_VECTOR_VIEW] = loc;
                    else if (name == "texture0")
                        asset->m_Shader.locs[SHADER_LOC_MAP_DIFFUSE] = loc;
                    else if (name == "colDiffuse")
                        asset->m_Shader.locs[SHADER_LOC_COLOR_DIFFUSE] = loc;
                    else if (name == "panorama")
                        asset->m_Shader.locs[SHADER_LOC_MAP_ALBEDO] = loc;
                    else if (name == "environmentMap")
                        asset->m_Shader.locs[SHADER_LOC_MAP_CUBEMAP] = loc;
                }
            }
        }
        return asset;
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to parse CHShader {}: {}", chshaderPath, e.what());
    }

    return nullptr;
}

int ShaderAsset::GetLocation(const std::string &name)
{
    if (m_UniformCache.find(name) != m_UniformCache.end())
        return m_UniformCache[name];

    int loc = GetShaderLocation(m_Shader, name.c_str());
    m_UniformCache[name] = loc;
    return loc;
}

void ShaderAsset::SetUniform(int loc, const void *value, int type)
{
    SetShaderValue(m_Shader, loc, value, type);
}

void ShaderAsset::SetUniform(const std::string &name, const void *value, int type)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(m_Shader, loc, value, type);
}
} // namespace CHEngine
