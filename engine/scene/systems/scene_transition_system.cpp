#include "scene_transition_system.h"
#include "engine/core/profiler.h"
#include "engine/scene/components/ui/control_component.h"
#include "engine/scene/components/ui/scene_transition_component.h"

namespace Chained::SceneTransitionSystem
{

	std::optional<std::string> Update(entt::registry& reg)
	{
		CH_PROFILE_FUNCTION();

		auto transitionView = reg.view<SceneTransitionComponent>();
		for (auto entity : transitionView)
		{
			auto& transition = transitionView.get<SceneTransitionComponent>(entity);
			if (reg.all_of<UIControlComponent>(entity))
			{
				auto& widget = reg.get<UIControlComponent>(entity);
				if (widget.PressedThisFrame)
				{
					transition.Triggered = true;
				}
			}

			if (transition.Triggered && !transition.TargetScenePath.empty())
			{
				transition.Triggered = false;
				return transition.TargetScenePath;
			}
		}

		return std::nullopt;
	}

} // namespace Chained::SceneTransitionSystem
