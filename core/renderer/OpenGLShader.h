#ifndef OPENGLSHADER_H
#define OPENGLSHADER_H

#include "Shader.h"
#include <glad.h>

namespace CHEngine
{

class OpenGLShader : public Shader
{
public:
    OpenGLShader(const std::string &filepath);
    OpenGLShader(const std::string &name, const std::string &vertexSrc,
                 const std::string &fragmentSrc);
    virtual ~OpenGLShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetInt(const std::string &name, int value) override;
    virtual void SetIntArray(const std::string &name, int *values, uint32_t count) override;
    virtual void SetFloat(const std::string &name, float value) override;
    virtual void SetFloat2(const std::string &name, const Vector2 &value) override;
    virtual void SetFloat3(const std::string &name, const Vector3 &value) override;
    virtual void SetFloat4(const std::string &name, const Vector4 &value) override;
    virtual void SetMat4(const std::string &name, const Matrix &value) override;

    virtual const std::string &GetName() const override;

    void UploadUniformInt(const std::string &name, int value);
    void UploadUniformIntArray(const std::string &name, int *values, uint32_t count);

    void UploadUniformFloat(const std::string &name, float value);
    void UploadUniformFloat2(const std::string &name, const Vector2 &value);
    void UploadUniformFloat3(const std::string &name, const Vector3 &value);
    void UploadUniformFloat4(const std::string &name, const Vector4 &value);

    void UploadUniformMat3(const std::string &name, const Matrix &matrix);
    void UploadUniformMat4(const std::string &name, const Matrix &matrix);

private:
    std::string ReadFile(const std::string &filepath);
    std::unordered_map<GLenum, std::string> PreProcess(const std::string &source);
    void Compile(const std::unordered_map<GLenum, std::string> &shaderSources);

private:
    uint32_t m_RendererID;
    std::string m_Name;
};

} // namespace CHEngine

#endif // OPENGLSHADER_H
