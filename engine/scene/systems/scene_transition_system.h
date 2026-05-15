#ifndef CH_SCENE_TRANSITION_SYSTEM_H
#define CH_SCENE_TRANSITION_SYSTEM_H

#include "engine/scene/scene_system.h"

namespace CHEngine
{
    class SceneTransitionSystem : public ISceneSystem
    {
    public:
        virtual void OnUpdate(Scene* scene, Timestep ts) override;
    };
}

#endif // CH_SCENE_TRANSITION_SYSTEM_H
