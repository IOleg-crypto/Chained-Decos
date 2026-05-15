#include "scene_transition_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/components/scene_transition_component.h"
#include "engine/scene/components/control_component.h"
#include "engine/core/application.h"

namespace CHEngine
{
    void SceneTransitionSystem::OnUpdate(Scene* scene, Timestep ts)
    {
        auto& registry = scene->GetRegistry();

        // 1. Process explicit triggers
        auto view = registry.view<SceneTransitionComponent>();
        for (auto entity : view)
        {
            auto& transition = view.get<SceneTransitionComponent>(entity);

            // 2. Automate UI actions: if it's a button and it's pressed, trigger the transition
            if (registry.all_of<WidgetComponent>(entity))
            {
                auto& widget = registry.get<WidgetComponent>(entity);
                if (widget.PressedThisFrame)
                {
                    transition.Triggered = true;
                }
            }

            if (transition.Triggered && !transition.TargetScenePath.empty())
            {
                SceneChangeRequestEvent ev(transition.TargetScenePath);
                Application::Get().OnEvent(ev);
                
                // Clear trigger to avoid multiple events in one frame (though scene should change)
                transition.Triggered = false;
            }
        }
    }
}
