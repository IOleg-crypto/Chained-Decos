#include "engine/graphics/assets/shader_asset.h"
#include "engine/core/log.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{
ShaderAsset::~ShaderAsset()
{
    if (m_Shader.id > 0)
    {
        glDeleteProgram(m_Shader.id);
    }
}

int ShaderAsset::GetLocation(const std::string& name)
{
    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end())
    {
        return it->second;
    }

    int location = glGetUniformLocation(m_Shader.id, name.c_str());
    m_UniformCache[name] = location;
    return location;
}

void ShaderAsset::SetUniform(int location, const void* value, int type)
{
    // type mapping from legacy raylib constants if needed, or we just use type-safe methods
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
    int location = GetLocation(name);
    if (location >= 0) glProgramUniform1f(m_Shader.id, location, value);
}

void ShaderAsset::SetInt(const std::string& name, int value)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniform1i(m_Shader.id, location, value);
}

void ShaderAsset::SetVec2(const std::string& name, const glm::vec2& value)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniform2fv(m_Shader.id, location, 1, glm::value_ptr(value));
}

void ShaderAsset::SetVec3(const std::string& name, const glm::vec3& value)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniform3fv(m_Shader.id, location, 1, glm::value_ptr(value));
}

void ShaderAsset::SetVec4(const std::string& name, const glm::vec4& value)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniform4fv(m_Shader.id, location, 1, glm::value_ptr(value));
}

void ShaderAsset::SetColor(const std::string& name, const Color& value)
{
    glm::vec4 c = {value.r / 255.0f, value.g / 255.0f, value.b / 255.0f, value.a / 255.0f};
    SetVec4(name, c);
}

void ShaderAsset::SetMatrix(const std::string& name, const glm::mat4& value)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniformMatrix4fv(m_Shader.id, location, 1, GL_FALSE, glm::value_ptr(value));
}


void ShaderAsset::SetMatrices(const std::string& name, const glm::mat4* values, int count)
{
    int location = GetLocation(name);
    if (location >= 0) glProgramUniformMatrix4fv(m_Shader.id, location, count, GL_FALSE, glm::value_ptr(values[0]));
}

} // namespace CHEngine
