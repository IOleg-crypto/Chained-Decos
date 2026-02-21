#ifndef CH_SCENE_SCRIPT_H
#define CH_SCENE_SCRIPT_H

#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scriptable_entity.h"

namespace CHEngine
{
CH_SCRIPT(SceneScript){public : CH_START(){
    CH_CORE_INFO("SceneScript: Initialized on entity '{}'", GetEntity().GetComponent<TagComponent>().Tag);
}

CH_UPDATE(dt)
{
    if (HasComponent<ButtonControl>())
    {
        auto& button = GetComponent<ButtonControl>();
        if (button.PressedThisFrame)
        {
            CH_CORE_INFO("SceneScript: Button '{}' pressed!", GetEntity().GetComponent<TagComponent>().Tag);

            std::string targetScene = "";

            // 1. Try to get path from SceneTransitionComponent if it exists on the same entity
            if (HasComponent<SceneTransitionComponent>())
            {
                auto& transition = GetComponent<SceneTransitionComponent>();
                targetScene = transition.TargetScenePath;
                CH_CORE_INFO("SceneScript: Found SceneTransitionComponent, target: {}", targetScene);
            }

            // 2. Fallback to hardcoded path if empty
            if (targetScene.empty())
            {
                targetScene = "scenes/main_menu.chscene"; // Default fallback
                CH_CORE_WARN("SceneScript: No TargetScenePath found, using fallback: {}", targetScene);
            }

            SceneChangeRequestEvent e(targetScene);
            Application::Get().OnEvent(e);
        }
    }
}
}
;
} // namespace CHEngine

#endif // CH_SCENE_SCRIPT_H
