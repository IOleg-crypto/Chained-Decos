#ifndef CH_SCENE_SYSTEMS_IMPL_H
#define CH_SCENE_SYSTEMS_IMPL_H

#include "engine/core/service_locator.h"
#include "engine/physics/physics_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_system.h"
#include "engine/scene/systems/animation_system.h"
#include "engine/scene/systems/hierarchy_system.h"
#include "engine/scene/systems/scene_audio_system.h"
#include "engine/physics/default_physics_world.h"
#include "engine/physics/physics.h"

namespace CHEngine
{
class HierarchySystemImpl : public ISceneSystem
{
public:
    void OnUpdate(Scene* scene, Timestep ts) override
    {
        HierarchySystem::Update(scene);
    }
    void OnUpdateEditor(Scene* scene, Timestep ts) override
    {
        HierarchySystem::Update(scene);
    }
};

class AnimationSystemImpl : public ISceneSystem
{
public:
    void OnUpdate(Scene* scene, Timestep ts) override
    {
        AnimationSystem::Update(scene, ts);
    }
};

class PhysicsSystemImpl : public ISceneSystem
{
public:
    PhysicsSystemImpl()
    {
        m_PhysicsWorld = std::make_unique<DefaultPhysicsWorld>();
    }

    void RegisterObservers(entt::registry& reg) override
    {
        // Provide access to the physics world via registry context for other systems/helpers
        reg.ctx().emplace<IPhysicsWorld*>(m_PhysicsWorld.get());
    }

    void OnUpdate(Scene* scene, Timestep ts) override
    {
        ServiceLocator::Get<PhysicsSystem>().Update(scene, ts, true);
    }
    void OnUpdateEditor(Scene* scene, Timestep ts) override
    {
        ServiceLocator::Get<PhysicsSystem>().Update(scene, ts, false);
    }
    void OnRuntimeStop(Scene* scene) override
    {
        Physics::ClearContext(scene);
    }

private:
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
};

class AudioSystemImpl : public ISceneSystem
{
public:
    void OnUpdate(Scene* scene, Timestep ts) override
    {
        SceneAudioSystem::Update(scene, ts);
    }
    void OnRuntimeStop(Scene* scene) override
    {
        SceneAudioSystem::OnRuntimeStop(scene);
    }
};
} // namespace CHEngine

#endif // CH_SCENE_SYSTEMS_IMPL_H
