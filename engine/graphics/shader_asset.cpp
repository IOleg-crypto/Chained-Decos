#include "shader_asset.h"
#include "engine/core/log.h"
#include "rlgl.h"

namespace CHEngine
{
ShaderAsset::~ShaderAsset()
{
    if (m_Shader.id > 0)
    {
        UnloadShader(m_Shader);
    }
}

int ShaderAsset::GetLocation(const std::string& name)
{
    if (m_UniformCache.find(name) != m_UniformCache.end())
    {
        return m_UniformCache[name];
    }

    int location = GetShaderLocation(m_Shader, name.c_str());
    m_UniformCache[name] = location;
    return location;
}

void ShaderAsset::SetUniform(int location, const void* value, int type)
{
    SetShaderValue(m_Shader, location, value, type);
}

void ShaderAsset::SetUniform(const std::string& name, const void* value, int type)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        SetShaderValue(m_Shader, location, value, type);
    }
}

// Type-safe helper methods
void ShaderAsset::SetFloat(const std::string& name, float value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        SetShaderValue(m_Shader, location, &value, SHADER_UNIFORM_FLOAT);
    }
}

void ShaderAsset::SetInt(const std::string& name, int value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        SetShaderValue(m_Shader, location, &value, SHADER_UNIFORM_INT);
    }
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
    {
        SetShaderValueMatrix(m_Shader, location, value);
    }
}

void ShaderAsset::SetMatrices(const std::string& name, const Matrix* values, int count)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        // SetShaderValueV doesn't seem to support matrices in this Raylib version
        // We use rlgl directly. rlEnableShader handles state caching internally.
        rlEnableShader(m_Shader.id);
        rlSetUniformMatrices(location, values, count);
    }
}
} // namespace CHEngine
