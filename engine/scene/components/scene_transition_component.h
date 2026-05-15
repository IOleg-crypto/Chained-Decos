#ifndef CH_SCENE_TRANSITION_COMPONENT_H
#define CH_SCENE_TRANSITION_COMPONENT_H

#include "engine/core/reflection.h"
#include <string>

namespace CHEngine
{
    // Component attached to entities (usually buttons) that should trigger a scene load.
    // This allows us to handle transitions natively without C# script overhead.
    struct SceneTransitionComponent
    {
        std::string TargetScenePath; // The relative path to the .chscene file to load
        bool Triggered = false;      // Can be set manually or via automated UI events

        SceneTransitionComponent() = default;
        SceneTransitionComponent(const std::string& path)
            : TargetScenePath(path)
        {
        }

        CH_REFLECT_BEGIN(SceneTransitionComponent)
            CH_FILE_NAMED(props, "Target Scene Path", TargetScenePath, "chscene");
            CH_PROP(props, Triggered);
        CH_REFLECT_END()
    };
}

#endif // CH_SCENE_TRANSITION_COMPONENT_H
