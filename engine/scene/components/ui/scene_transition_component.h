#ifndef CH_SCENE_TRANSITION_COMPONENT_H
#define CH_SCENE_TRANSITION_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include <string>

namespace Chained
{
	// Component attached to entities (usually buttons) that should trigger a scene load.
	// This allows us to handle transitions natively without C# script overhead.
	struct SceneTransitionComponent
	{
		std::string TargetScenePath; // The relative path to the .chscene file to load
		bool Triggered = false;		 // Can be set manually or via automated UI events

		static const char* GetStaticName()
		{
			return "SceneTransitionComponent";
		}

		struct UI
		{
			UIMeta TargetScenePath = {.Hint = PropertyMeta::WidgetHint::FilePicker, .Extensions = ".chscene"};
			UIMeta Triggered = {.Hint = PropertyMeta::WidgetHint::Checkbox,
								.Tooltip = "Whether the scene transition has been triggered"};
		};
	};

	CH_MARK_RFL(SceneTransitionComponent);

} // namespace Chained

#endif // CH_SCENE_TRANSITION_COMPONENT_H
