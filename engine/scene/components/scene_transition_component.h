#ifndef CH_SCENE_TRANSITION_COMPONENT_H
#define CH_SCENE_TRANSITION_COMPONENT_H

#include "engine/core/reflection.h"
#include <string>

namespace CHEngine
{
    struct SceneTransitionComponent
    {
        std::string TargetScenePath;
        bool Triggered = false;

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
