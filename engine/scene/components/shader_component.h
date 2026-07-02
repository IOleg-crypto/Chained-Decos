#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/pipeline/renderer_types.h"

namespace Chained
{
struct ShaderComponent
{
    AssetHandle ShaderHandle = AssetHandle(0);
    std::string ShaderPath;
    std::vector<ShaderUniform> Uniforms;
    bool Enabled = true;

    static const char* GetStaticName() { return "ShaderComponent"; }

    
    struct UI
    {
        UIMeta ShaderPath = {.Hint = PropertyMeta::WidgetHint::FilePicker, .Extensions = ".glsl,.vs,.fs,.vert,.frag"};
        UIMeta Enabled = {.Tooltip = "Увімкнути використання цього кастомного шейдера"};
    };
};

CH_MARK_RFL(ShaderComponent);

} // namespace Chained

#endif // CH_SHADER_COMPONENT_H
