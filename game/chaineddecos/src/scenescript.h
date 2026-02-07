#ifndef CH_SCENE_SCRIPT_H
#define CH_SCENE_SCRIPT_H

#include "engine/core/application.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "engine/scene/scene_events.h"

namespace CHEngine
{
    CH_SCRIPT(SceneScript)
    {
    public:
        CH_START()
        {
            CH_CORE_INFO("SceneScript: Initialized");
        }

        CH_UPDATE(dt)
        {
            if (HasComponent<ButtonControl>())
            {
                auto &button = GetComponent<ButtonControl>();
                if (button.PressedThisFrame)
                {
                    CH_CORE_INFO("SceneScript: Button pressed, requesting scene change...");
                    SceneChangeRequestEvent e(PROJECT_ROOT_DIR "/game/chaineddecos/assets/scenes/Untitled1.chscene");
                    Application::Get().OnEvent(e);
                }
            }
        }
    };
} // namespace CHEngine

#endif // CH_SCENE_SCRIPT_H
