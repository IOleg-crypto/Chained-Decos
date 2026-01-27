#ifndef CH_SCENE_SCRIPT_H
#define CH_SCENE_SCRIPT_H

#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"

namespace CHEngine
{
CH_SCRIPT(SceneScript){CH_START(){CH_CORE_INFO("SceneScript: Initialized");
}

CH_UPDATE(dt)
{
    if (HasComponent<ButtonWidget>())
    {
        auto &button = GetComponent<ButtonWidget>();
        if (button.PressedThisFrame)
        {
            CH_CORE_INFO("SceneScript: Button pressed, requesting scene change...");
            Application::Get().RequestSceneChange(
                PROJECT_ROOT_DIR "/game/chaineddecos/assets/scenes/Untitled1.chscene");
        }
    }
}
}
;
} // namespace CHEngine

#endif // CH_SCENE_SCRIPT_H