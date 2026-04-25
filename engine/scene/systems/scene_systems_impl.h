#ifndef CH_SCENE_SYSTEMS_IMPL_H
#define CH_SCENE_SYSTEMS_IMPL_H

#include "engine/scene/scene_system.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/scene/systems/animation_system.h"
#include "engine/scene/systems/scene_audio_system.h"
#include "engine/physics/physics_system.h"
#include "engine/core/service_locator.h"

namespace CHEngine
{
    class HierarchySystemImpl : public ISceneSystem
    {
    public:
        void OnUpdate(Scene* scene, Timestep ts) override { HierarchySystem::Update(scene); }
        void OnUpdateEditor(Scene* scene, Timestep ts) override { HierarchySystem::Update(scene); }
    };

    class AnimationSystemImpl : public ISceneSystem
    {
    public:
        void OnUpdate(Scene* scene, Timestep ts) override { AnimationSystem::Update(scene, ts); }
    };

    class PhysicsSystemImpl : public ISceneSystem
    {
    public:
        void OnUpdate(Scene* scene, Timestep ts) override 
        { 
            ServiceLocator::Get<PhysicsSystem>().Update(scene, ts, true); 
        }
        void OnUpdateEditor(Scene* scene, Timestep ts) override 
        { 
            ServiceLocator::Get<PhysicsSystem>().Update(scene, ts, false); 
        }
    };

    class AudioSystemImpl : public ISceneSystem
    {
    public:
        void OnUpdate(Scene* scene, Timestep ts) override { SceneAudioSystem::Update(scene, ts); }
    };
}

#endif // CH_SCENE_SYSTEMS_IMPL_H
