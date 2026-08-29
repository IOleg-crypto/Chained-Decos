#ifndef CH_MODEL_COMPONENT_H
#define CH_MODEL_COMPONENT_H

#include "engine/assets/asset.h"
#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include <string>
#include <vector>

namespace Chained
{
	struct ModelComponent
	{
		AssetHandle ModelHandle = AssetHandle(0);
		std::string ModelPath;
		uint64_t ModelUUID = 0;
		std::vector<std::string> MaterialPaths;

		static const char* GetStaticName()
		{
			return "ModelComponent";
		}

		struct UI
		{
			UIMeta ModelHandle = {.ReadOnly = true, .Transient = true};
			UIMeta ModelPath = {.Hint = PropertyMeta::WidgetHint::FilePicker,
								.Extensions = ".glb,.gltf,.obj",
								.Tooltip = "Path to the 3D model file"};
			UIMeta ModelUUID = {.ReadOnly = true};
		};
	};

	CH_MARK_RFL(ModelComponent);

} // namespace Chained

#endif // CH_MODEL_COMPONENT_H