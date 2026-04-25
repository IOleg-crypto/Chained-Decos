#ifndef CH_ISCENE_SYSTEM_H
#define CH_ISCENE_SYSTEM_H

#include "engine/core/timestep.h"

namespace CHEngine
{
    class Scene;

    class ISceneSystem
    {
    public:
        virtual ~ISceneSystem() = default;

        // Called every frame during runtime/simulation
        virtual void OnUpdate(Scene* scene, Timestep ts) = 0;

        // Called when the scene starts simulate/play
        virtual void OnRuntimeStart(Scene* scene) {}

        // Called when the scene stops simulate/play
        virtual void OnRuntimeStop(Scene* scene) {}

        // Optional: Called for editor-specific updates
        virtual void OnUpdateEditor(Scene* scene, Timestep ts) { OnUpdate(scene, ts); }
    };
}

#endif // CH_ISCENE_SYSTEM_H
