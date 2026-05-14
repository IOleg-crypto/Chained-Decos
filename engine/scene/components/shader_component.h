#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <glm/glm.hpp>
#include <string>

namespace CHEngine
{
class AssetManager;

struct ShaderComponent
{
    AssetHandle ShaderHandle = AssetHandle(0);
    std::string ShaderPath;
    std::vector<ShaderUniform> Uniforms;
    bool Enabled = true;

    ShaderComponent() = default;
    ShaderComponent(const ShaderComponent&) = default;

    CH_REFLECT_BEGIN(ShaderComponent)
        CH_HEADER(props, "Shader Asset");
        if (props.GetMode() != CHEngine::ReflectionMode::UI)
        {
            CH_HANDLE(props, ShaderHandle);
        }

        if (CH_FILE(props, ShaderPath, "glsl,chshader"))
        {
            ShaderHandle = AssetHandle(0);
        }

        if (props.GetMode() != CHEngine::ReflectionMode::UI)
        {
            CH_SEQUENCE_EX(props, Uniforms, false);
        }

        CH_ACTION(props, "Refresh Uniforms", [&]() { ShaderHandle = AssetHandle(0); });
        CH_ACTION(props, "Clear Overrides", [&]() { Uniforms.clear(); ShaderHandle = AssetHandle(0); });
        CH_SEQUENCE_EX(props, Uniforms, false);
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_SHADER_COMPONENT_H
