#ifndef CH_SCENE_TRANSITION_COMPONENT_H
#define CH_SCENE_TRANSITION_COMPONENT_H

#include "engine/core/reflection_rfl.h"
#include <string>

namespace CHEngine
{
    // Component attached to entities (usually buttons) that should trigger a scene load.
    // This allows us to handle transitions natively without C# script overhead.
    struct SceneTransitionComponent
    {
        std::string TargetScenePath; // The relative path to the .chscene file to load
        bool Triggered = false;      // Can be set manually or via automated UI events

        static const char* GetStaticName() { return "SceneTransitionComponent"; }
    };

    CH_MARK_RFL(SceneTransitionComponent);

}

#endif // CH_SCENE_TRANSITION_COMPONENT_H
