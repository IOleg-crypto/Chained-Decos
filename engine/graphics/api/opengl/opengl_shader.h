#ifndef CH_OPENGL_SHADER_H
#define CH_OPENGL_SHADER_H

#include "engine/graphics/api/shader.h"
#include <unordered_map>

namespace CHEngine {

class OpenGLShader : public Shader
{
public:
    OpenGLShader(const std::string& vsSource, const std::string& fsSource);
    virtual ~OpenGLShader();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetRendererID() const override { return m_RendererID; }

    virtual void SetFloat(const std::string& name, float value) override;
    virtual void SetInt(const std::string& name, int value) override;
    virtual void SetVec2(const std::string& name, const glm::vec2& value) override;
    virtual void SetVec3(const std::string& name, const glm::vec3& value) override;
    virtual void SetVec4(const std::string& name, const glm::vec4& value) override;
    virtual void SetColor(const std::string& name, const Color& value) override;
    virtual void SetMatrix(const std::string& name, const glm::mat4& value) override;
    virtual void SetMatrices(const std::string& name, const glm::mat4* values, int count) override;

private:
    int GetLocation(const std::string& name);

private:
    uint32_t m_RendererID = 0;
    std::unordered_map<std::string, int> m_UniformCache;
};

} // namespace CHEngine

#endif // CH_OPENGL_SHADER_H
