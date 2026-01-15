#include "opengl_shader.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include "opengl_shader.h"
#include <array>
#include <cstring>
#include <fstream>
#include <raymath.h>
#include <vector>


namespace CHEngine
{

static GLenum ShaderTypeFromString(const std::string &type)
{
    if (type == "vertex")
        return GL_VERTEX_SHADER;
    if (type == "fragment" || type == "pixel")
        return GL_FRAGMENT_SHADER;

    CH_CORE_ASSERT(false, "Unknown shader type!");
    return 0;
}

OpenGLShader::OpenGLShader(const std::string &filepath)
{
    std::string source = ReadFile(filepath);
    auto shaderSources = PreProcess(source);
    Compile(shaderSources);

    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
}

OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSrc,
                           const std::string &fragmentSrc)
    : m_Name(name)
{
    std::unordered_map<GLenum, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSrc;
    sources[GL_FRAGMENT_SHADER] = fragmentSrc;
    Compile(sources);
}

OpenGLShader::~OpenGLShader()
{
    glDeleteProgram(m_RenderID);
}

std::string OpenGLShader::ReadFile(const std::string &filepath)
{
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&result[0], result.size());
        in.close();
    }
    else
    {
        CH_CORE_ERROR("Could not open file '{0}'", filepath);
    }

    return result;
}

std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string &source)
{
    std::unordered_map<GLenum, std::string> shaderSources;

    const char *typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken, 0);
    while (pos != std::string::npos)
    {
        size_t eol = source.find_first_of("\r\n", pos);
        CH_CORE_ASSERT(eol != std::string::npos, "Syntax error");
        size_t begin = pos + typeTokenLength + 1;
        std::string type = source.substr(begin, eol - begin);
        CH_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        pos = source.find(typeToken, nextLinePos);
        shaderSources[ShaderTypeFromString(type)] = source.substr(
            nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() : pos));
    }

    return shaderSources;
}

void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string> &shaderSources)
{
    GLuint program = glCreateProgram();
    CH_CORE_ASSERT(shaderSources.size() <= 2, "We only support 2 shaders for now");
    std::array<GLenum, 2> glShaderIDs;
    int glShaderIDIndex = 0;
    for (auto &kv : shaderSources)
    {
        GLenum type = kv.first;
        const std::string &source = kv.second;

        GLuint shader = glCreateShader(type);

        const GLchar *sourceCStr = source.c_str();
        glShaderSource(shader, 1, &sourceCStr, 0);

        glCompileShader(shader);

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

            glDeleteShader(shader);

            CH_CORE_ERROR("{0}", infoLog.data());
            CH_CORE_ASSERT(false, "Shader compilation failure!");
            break;
        }

        glAttachShader(program, shader);
        glShaderIDs[glShaderIDIndex++] = shader;
    }

    m_RenderID = program;

    // Link our program
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

        glDeleteProgram(program);
        for (auto id : glShaderIDs)
            glDeleteShader(id);

        CH_CORE_ERROR("{0}", infoLog.data());
        CH_CORE_ASSERT(false, "Shader link failure!");
        return;
    }

    for (auto id : glShaderIDs)
        glDetachShader(program, id);
}

void OpenGLShader::Bind() const
{
    glUseProgram(m_RenderID);
}

void OpenGLShader::Unbind() const
{
    glUseProgram(0);
}

void OpenGLShader::SetInt(const std::string &name, int value)
{
    UploadUniformInt(name, value);
}

void OpenGLShader::SetIntArray(const std::string &name, int *values, uint32_t count)
{
    UploadUniformIntArray(name, values, count);
}

void OpenGLShader::SetFloat(const std::string &name, float value)
{
    UploadUniformFloat(name, value);
}

void OpenGLShader::SetFloat2(const std::string &name, const Vector2 &value)
{
    UploadUniformFloat2(name, value);
}

void OpenGLShader::SetFloat3(const std::string &name, const Vector3 &value)
{
    UploadUniformFloat3(name, value);
}

void OpenGLShader::SetFloat4(const std::string &name, const Vector4 &value)
{
    UploadUniformFloat4(name, value);
}

void OpenGLShader::SetMat4(const std::string &name, const Matrix &value)
{
    UploadUniformMat4(name, value);
}

void OpenGLShader::UploadUniformInt(const std::string &name, int value)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform1i(location, value);
}

void OpenGLShader::UploadUniformIntArray(const std::string &name, int *values, uint32_t count)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform1iv(location, count, values);
}

void OpenGLShader::UploadUniformFloat(const std::string &name, float value)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform1f(location, value);
}

void OpenGLShader::UploadUniformFloat2(const std::string &name, const Vector2 &value)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform2f(location, value.x, value.y);
}

void OpenGLShader::UploadUniformFloat3(const std::string &name, const Vector3 &value)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform3f(location, value.x, value.y, value.z);
}

void OpenGLShader::UploadUniformFloat4(const std::string &name, const Vector4 &value)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniform4f(location, value.x, value.y, value.z, value.w);
}

void OpenGLShader::UploadUniformMat3(const std::string &name, const Matrix &matrix)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    // Mat3 is rare in this engine, but could be implemented by extracting top-left 3x3
}

void OpenGLShader::UploadUniformMat4(const std::string &name, const Matrix &matrix)
{
    GLint location = glGetUniformLocation(m_RenderID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix.m0);
}

const std::string &OpenGLShader::GetName() const
{
    return m_Name;
}

} // namespace CHEngine
