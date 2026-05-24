#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "engine/core/reflection_rfl.h"
#include "engine/graphics/pipeline/renderer_types.h"

namespace CHEngine
{
class AssetManager;

struct ShaderComponent
{
    AssetHandle ShaderHandle = AssetHandle(0);
    std::string ShaderPath;
    std::vector<ShaderUniform> Uniforms;
    bool Enabled = true;

    static const char* GetStaticName() { return "ShaderComponent"; }
};

CH_MARK_RFL(ShaderComponent);

} // namespace CHEngine

#endif // CH_SHADER_COMPONENT_H
