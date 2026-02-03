#include "shader_asset.h"
#include "engine/core/log.h"
#include "engine/graphics/asset_manager.h"
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
        asset->SetState(AssetState::Ready);
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

        std::string vsPath = AssetManager::ResolvePath(vsRel);
        std::string fsPath = AssetManager::ResolvePath(fsRel);
        
        CH_CORE_INFO("ShaderAsset: Loading from {}", chshaderPath);
        CH_CORE_INFO("  VertexShader: {} -> {} (exists={})", vsRel, vsPath, std::filesystem::exists(vsPath));
        CH_CORE_INFO("  FragmentShader: {} -> {} (exists={})", fsRel, fsPath, std::filesystem::exists(fsPath));

        std::shared_ptr<ShaderAsset> asset = Load(vsPath, fsPath);
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
                    else if (name == "boneMatrices")
                        asset->m_Shader.locs[SHADER_LOC_BONE_MATRICES] = loc;
                }
                
                // Also auto-map attributes if names follow Raylib convention
                asset->m_Shader.locs[SHADER_LOC_VERTEX_BONEIDS] = GetShaderLocationAttrib(asset->m_Shader, "vertexBoneIds");
                asset->m_Shader.locs[SHADER_LOC_VERTEX_BONEWEIGHTS] = GetShaderLocationAttrib(asset->m_Shader, "vertexBoneWeights");
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
void ShaderAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    auto loaded = Load(path);
    if (loaded)
    {
        m_Shader = loaded->m_Shader;
        loaded->m_Shader.id = 0; // Prevent double unload
        m_UniformCache = std::move(loaded->m_UniformCache);
        SetState(AssetState::Ready);
    }
    else
    {
        SetState(AssetState::Failed);
    }
}

void ShaderAsset::UploadToGPU()
{
    // Shader is usually loaded on main thread or already uploaded in LoadFromFile (via Raylib)
}

// Type-safe helper methods
void ShaderAsset::SetFloat(const std::string& name, float value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(m_Shader, loc, &value, SHADER_UNIFORM_FLOAT);
}

void ShaderAsset::SetInt(const std::string& name, int value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValue(m_Shader, loc, &value, SHADER_UNIFORM_INT);
}

void ShaderAsset::SetVec2(const std::string& name, const Vector2& value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
    {
        float v[2] = {value.x, value.y};
        SetShaderValue(m_Shader, loc, v, SHADER_UNIFORM_VEC2);
    }
}

void ShaderAsset::SetVec3(const std::string& name, const Vector3& value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
    {
        float v[3] = {value.x, value.y, value.z};
        SetShaderValue(m_Shader, loc, v, SHADER_UNIFORM_VEC3);
    }
}

void ShaderAsset::SetVec4(const std::string& name, const Vector4& value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
    {
        float v[4] = {value.x, value.y, value.z, value.w};
        SetShaderValue(m_Shader, loc, v, SHADER_UNIFORM_VEC4);
    }
}

void ShaderAsset::SetColor(const std::string& name, const Color& value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
    {
        float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
        SetShaderValue(m_Shader, loc, c, SHADER_UNIFORM_VEC4);
    }
}

void ShaderAsset::SetMatrix(const std::string& name, const Matrix& value)
{
    int loc = GetLocation(name);
    if (loc >= 0)
        SetShaderValueMatrix(m_Shader, loc, value);
}

} // namespace CHEngine
