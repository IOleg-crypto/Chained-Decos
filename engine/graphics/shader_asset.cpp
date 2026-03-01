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
    // Caching for basic types (float, vec2, vec3, vec4, int)
    int floatCount = 0;
    switch (type)
    {
        case SHADER_UNIFORM_FLOAT: floatCount = 1; break;
        case SHADER_UNIFORM_VEC2:  floatCount = 2; break;
        case SHADER_UNIFORM_VEC3:  floatCount = 3; break;
        case SHADER_UNIFORM_VEC4:  floatCount = 4; break;
        case SHADER_UNIFORM_INT:   floatCount = 1; break;
    }

    if (floatCount > 0)
    {
        const float* fptr = (const float*)value;
        auto& cached = m_ValueCache[location];
        bool changed = cached.size() != (size_t)floatCount;
        if (!changed)
        {
            for (int i = 0; i < floatCount; i++)
            {
                if (cached[i] != fptr[i])
                {
                    changed = true;
                    break;
                }
            }
        }

        if (!changed) return;

        cached.assign(fptr, fptr + floatCount);
    }

    SetShaderValue(m_Shader, location, value, type);
}

void ShaderAsset::SetUniform(const std::string& name, const void* value, int type)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        SetUniform(location, value, type);
    }
}

// Type-safe helper methods
void ShaderAsset::SetFloat(const std::string& name, float value)
{
    SetUniform(name, &value, SHADER_UNIFORM_FLOAT);
}

void ShaderAsset::SetInt(const std::string& name, int value)
{
    // Reuse SetUniform for int as well
    SetUniform(name, &value, SHADER_UNIFORM_INT);
}

void ShaderAsset::SetVec2(const std::string& name, const Vector2& value)
{
    SetUniform(name, &value, SHADER_UNIFORM_VEC2);
}

void ShaderAsset::SetVec3(const std::string& name, const Vector3& value)
{
    SetUniform(name, &value, SHADER_UNIFORM_VEC3);
}

void ShaderAsset::SetVec4(const std::string& name, const Vector4& value)
{
    SetUniform(name, &value, SHADER_UNIFORM_VEC4);
}

void ShaderAsset::SetColor(const std::string& name, const Color& value)
{
    float c[4] = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
    SetUniform(name, c, SHADER_UNIFORM_VEC4);
}

void ShaderAsset::SetMatrix(const std::string& name, const Matrix& value)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        // For matrices we could also cache, but let's start with basic types
        SetShaderValueMatrix(m_Shader, location, value);
    }
}

void ShaderAsset::SetMatrices(const std::string& name, const Matrix* values, int count)
{
    int location = GetLocation(name);
    if (location >= 0)
    {
        rlEnableShader(m_Shader.id);
        rlSetUniformMatrices(location, values, count);
    }
}
} // namespace CHEngine
