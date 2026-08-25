#ifndef CH_SHADER_COMPONENT_H
#define CH_SHADER_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/assets/asset.h"

namespace Chained
{
	struct ShaderComponent
	{
		AssetHandle ShaderHandle = AssetHandle(0);
		std::string ShaderPath;
		uint64_t ShaderUUID = 0;
		std::vector<ShaderUniform> Uniforms;
		bool Enabled = true;

		static const char* GetStaticName()
		{
			return "ShaderComponent";
		}

		// Declarative UI layout metadata for compile-time reflection
		struct UI
		{
			UIMeta ShaderPath = {.Hint = PropertyMeta::WidgetHint::FilePicker,
								 .Tooltip = "Path to the source GLSL shader file (vertex, fragment, or unified code)",
								 .Extensions = ".glsl,.vs,.fs,.vert,.frag"};

			UIMeta ShaderUUID = {.ReadOnly = true};

			UIMeta Enabled = {.Tooltip = "Toggle whether this custom shader is active and applied during rendering"};

			// Runtime/Internal state shown in the UI for debugging purposes
			UIMeta ShaderHandle = {.ReadOnly = true,
								   .Transient = true,
								   .Tooltip = "Internal engine asset handle assigned to the compiled shader program"};
		};
	};

	CH_MARK_RFL(ShaderComponent);

} // namespace Chained

#endif // CH_SHADER_COMPONENT_H