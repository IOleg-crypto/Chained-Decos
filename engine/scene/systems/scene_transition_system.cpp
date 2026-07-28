#include "scene_transition_system.h"
#include "engine/core/profiler.h"
#include "engine/scene/components/control_component.h"
#include "engine/scene/components/scene_transition_component.h"
#include "engine/scene/scene_events.h"

namespace Chained::SceneTransitionSystem
{

void Update(entt::registry& reg, const EventCallbackFn& callback)
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
            SceneChangeRequestEvent ev(transition.TargetScenePath);
            if (callback)
            {
                callback(ev);
            }
            else
            {
                CH_CORE_WARN("Scene transition triggered but no EventCallback bound!");
            }

            transition.Triggered = false;
        }
    }
}

} // namespace Chained::SceneTransitionSystem
