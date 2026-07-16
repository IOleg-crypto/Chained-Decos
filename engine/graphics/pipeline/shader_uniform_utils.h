#ifndef CH_SHADER_UNIFORM_UTILS_H
#define CH_SHADER_UNIFORM_UTILS_H

#include "engine/graphics/api/renderer_types.h"
#include "engine/graphics/api/shader.h"
#include <variant>

namespace Chained
{
inline void ApplyShaderUniforms(Shader* shader, const std::vector<ShaderUniform>& uniforms)
{
    for (const auto& u : uniforms)
    {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, float>)
                {
                    shader->SetFloat(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec2>)
                {
                    shader->SetVec2(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec3>)
                {
                    shader->SetVec3(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, glm::vec4>)
                {
                    shader->SetVec4(u.Name, arg);
                }
                else if constexpr (std::is_same_v<T, Color>)
                {
                    glm::vec4 colorVec = {arg.r / 255.0f, arg.g / 255.0f, arg.b / 255.0f, arg.a / 255.0f};
                    shader->SetVec4(u.Name, colorVec);
                }
            },
            u.Value);
    }
}
} // namespace Chained

#endif // CH_SHADER_UNIFORM_UTILS_H
