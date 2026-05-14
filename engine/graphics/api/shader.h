#ifndef CH_SHADER_H
#define CH_SHADER_H
#include "engine/core/ch_structures.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace CHEngine
{

// Abstract GPU shader interface used by backends and higher-level render code.
class Shader
{
public:
    virtual ~Shader() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual uint32_t GetRendererID() const = 0;

    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetInt(const std::string& name, int value) = 0;
    virtual void SetVec2(const std::string& name, const glm::vec2& value) = 0;
    virtual void SetVec3(const std::string& name, const glm::vec3& value) = 0;
    virtual void SetVec4(const std::string& name, const glm::vec4& value) = 0;
    virtual void SetColor(const std::string& name, const Color& value) = 0;
    virtual void SetMatrix(const std::string& name, const glm::mat4& value) = 0;
    virtual void SetMatrices(const std::string& name, const glm::mat4* values, int count) = 0;

    // Builds a shader from vertex and fragment source.
    static std::shared_ptr<Shader> Create(const std::string& vsSource, const std::string& fsSource);
};

} // namespace CHEngine
#endif
