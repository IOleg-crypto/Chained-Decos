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

        // Check all entities that have a transition component
        auto view = registry.view<SceneTransitionComponent>();
        for (auto entity : view)
        {
            auto& transition = view.get<SceneTransitionComponent>(entity);

            // Automated UI handling: if it's a button and was clicked, set the trigger
            if (registry.all_of<WidgetComponent>(entity))
            {
                auto& widget = registry.get<WidgetComponent>(entity);
                if (widget.PressedThisFrame)
                {
                    transition.Triggered = true;
                }
            }

            // If triggered, send a global event to the editor/runtime to switch scenes
            if (transition.Triggered && !transition.TargetScenePath.empty())
            {
                CH_INFO_ONCE("SceneTransitionSystem: Switching to scene {0}", transition.TargetScenePath);
                SceneChangeRequestEvent ev(transition.TargetScenePath);
                Application::Get().OnEvent(ev);
                
                // Clear trigger to prevent looping (scene change is handled async by the layer)
                transition.Triggered = false;
            }
        }
    }
}
