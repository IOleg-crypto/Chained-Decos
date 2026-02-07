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

std::shared_ptr<ShaderAsset> ShaderAsset::Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{
    std::filesystem::path vertexShaderAbsolute(vertexShaderPath);
    std::filesystem::path fragmentShaderAbsolute(fragmentShaderPath);

    Shader shader = ::LoadShader(vertexShaderAbsolute.string().c_str(), fragmentShaderAbsolute.string().c_str());
    if (shader.id > 0)
    {
        auto asset = std::make_shared<ShaderAsset>(shader);
        asset->SetPath(vertexShaderPath + "|" + fragmentShaderPath);
        asset->SetState(AssetState::Ready);
        return asset;
    }

    CH_CORE_ERROR("Failed to load shader: VS: {}, FS: {}", vertexShaderPath, fragmentShaderPath);
    return nullptr;
}

std::shared_ptr<ShaderAsset> ShaderAsset::Load(const std::string &chshaderPath)
{
    auto asset = std::make_shared<ShaderAsset>();
    asset->SetPath(chshaderPath);
    asset->LoadFromFile(chshaderPath);
    return asset->GetState() == AssetState::Ready ? asset : nullptr;
}

int ShaderAsset::GetLocation(const std::string &name)
{
    if (m_UniformCache.find(name) != m_UniformCache.end())
        return m_UniformCache[name];

    int location = GetShaderLocation(m_Shader, name.c_str());
    m_UniformCache[name] = location;
    return location;
}

void ShaderAsset::SetUniform(int location, const void *value, int type)
{
    SetShaderValue(m_Shader, location, value, type);
}

void ShaderAsset::SetUniform(const std::string &name, const void *value, int type)
{
    int location = GetLocation(name);
    if (location >= 0)
        SetShaderValue(m_Shader, location, value, type);
}
void ShaderAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    std::filesystem::path absolutePath(path);
    if (!std::filesystem::exists(absolutePath))
    {
        CH_CORE_ERROR("CHShader not found: {}", path);
        SetState(AssetState::Failed);
        return;
    }

    try
    {
        YAML::Node config = YAML::LoadFile(absolutePath.string());
        std::string vertexShaderRelative = config["VertexShader"].as<std::string>();
        std::string fragmentShaderRelative = config["FragmentShader"].as<std::string>();

        // Paths are relative to the chshader file
        std::filesystem::path basePath = std::filesystem::path(path).parent_path();
        std::string vertexShaderPath = (basePath / vertexShaderRelative).string();
        std::string fragmentShaderPath = (basePath / fragmentShaderRelative).string();
        
        CH_CORE_INFO("ShaderAsset: Loading from {}", path);
        CH_CORE_INFO("  VertexShader: {} -> {} (exists={})", vertexShaderRelative, vertexShaderPath, std::filesystem::exists(vertexShaderPath));
        CH_CORE_INFO("  FragmentShader: {} -> {} (exists={})", fragmentShaderRelative, fragmentShaderPath, std::filesystem::exists(fragmentShaderPath));

        // Load shader
        m_Shader = ::LoadShader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        
        if (m_Shader.id == 0)
        {
            CH_CORE_ERROR("Failed to load shader: VS: {}, FS: {}", vertexShaderPath, fragmentShaderPath);
            SetState(AssetState::Failed);
            return;
        }

        // Automatic uniform caching and Raylib standard mapping
        if (config["Uniforms"])
        {
            for (auto uniform : config["Uniforms"])
            {
                std::string name = uniform.as<std::string>();
                int location = GetLocation(name);

                // Map standard names to Raylib locs
                if (name == "mvp")
                    m_Shader.locs[SHADER_LOC_MATRIX_MVP] = location;
                else if (name == "matModel")
                    m_Shader.locs[SHADER_LOC_MATRIX_MODEL] = location;
                else if (name == "matNormal")
                    m_Shader.locs[SHADER_LOC_MATRIX_NORMAL] = location;
                else if (name == "matView")
                    m_Shader.locs[SHADER_LOC_MATRIX_VIEW] = location;
                else if (name == "matProjection")
                    m_Shader.locs[SHADER_LOC_MATRIX_PROJECTION] = location;
                else if (name == "viewPos")
                    m_Shader.locs[SHADER_LOC_VECTOR_VIEW] = location;
                else if (name == "texture0")
                    m_Shader.locs[SHADER_LOC_MAP_DIFFUSE] = location;
                else if (name == "colDiffuse")
                    m_Shader.locs[SHADER_LOC_COLOR_DIFFUSE] = location;
                else if (name == "panorama")
                    m_Shader.locs[SHADER_LOC_MAP_ALBEDO] = location;
                else if (name == "environmentMap")
                    m_Shader.locs[SHADER_LOC_MAP_CUBEMAP] = location;
                else if (name == "boneMatrices")
                    m_Shader.locs[SHADER_LOC_BONE_MATRICES] = location;
            }
            
            // Also auto-map attributes if names follow Raylib convention
            m_Shader.locs[SHADER_LOC_VERTEX_BONEIDS] = GetShaderLocationAttrib(m_Shader, "vertexBoneIds");
            m_Shader.locs[SHADER_LOC_VERTEX_BONEWEIGHTS] = GetShaderLocationAttrib(m_Shader, "vertexBoneWeights");
        }
        
        SetState(AssetState::Ready);
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to parse CHShader {}: {}", path, e.what());
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
    int location = GetLocation(name);
    if (location >= 0)
        SetShaderValue(m_Shader, location, &value, SHADER_UNIFORM_FLOAT);
}

void ShaderAsset::SetInt(const std::string& name, int value)
{
    int location = GetLocation(name);
    if (location >= 0)
        SetShaderValue(m_Shader, location, &value, SHADER_UNIFORM_INT);
}

void ShaderAsset::SetVec2(const std::string& name, const Vector2& value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        float v[2] = {value.x, value.y};
        SetShaderValue(m_Shader, location, v, SHADER_UNIFORM_VEC2);
    }
}

void ShaderAsset::SetVec3(const std::string& name, const Vector3& value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        float v[3] = {value.x, value.y, value.z};
        SetShaderValue(m_Shader, location, v, SHADER_UNIFORM_VEC3);
    }
}

void ShaderAsset::SetVec4(const std::string& name, const Vector4& value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        float v[4] = {value.x, value.y, value.z, value.w};
        SetShaderValue(m_Shader, location, v, SHADER_UNIFORM_VEC4);
    }
}

void ShaderAsset::SetColor(const std::string& name, const Color& value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
        SetShaderValue(m_Shader, location, c, SHADER_UNIFORM_VEC4);
    }
}

void ShaderAsset::SetMatrix(const std::string& name, const Matrix& value)
{
    int location = GetLocation(name);
    if (location >= 0)
        SetShaderValueMatrix(m_Shader, location, value);
}

} // namespace CHEngine
