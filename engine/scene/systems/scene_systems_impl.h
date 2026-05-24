#ifndef CH_SCENE_SYSTEMS_IMPL_H
#define CH_SCENE_SYSTEMS_IMPL_H

#include "engine/core/service_locator.h"
#include "engine/physics/default_physics_world.h"
#include "engine/physics/physics.h"
#include "engine/physics/physics_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_system.h"
#include "engine/scene/systems/scene_resource_manager.h"
#include "engine/scene/systems/hierarchy_system.h"
#include <memory>

namespace CHEngine
{
class SceneRuntimeUpdater : public ISceneSystem
{
public:
    SceneRuntimeUpdater()
        : m_PhysicsWorld(std::make_unique<DefaultPhysicsWorld>())
    {
    }

    SceneRuntimeUpdater(PhysicsSystem& physics)
        : m_PhysicsWorld(std::make_unique<DefaultPhysicsWorld>())
        , m_PhysicsSystem(&physics)
    {
    }

    void RegisterObservers(entt::registry& reg) override
    {
        reg.ctx().emplace<IPhysicsWorld*>(m_PhysicsWorld.get());
    }

    void OnUpdate(Scene* scene, Timestep ts) override
    {
        // Get consolidated resource manager from registry context
        SceneResourceManager* resMgr = nullptr;
        if (scene->GetRegistry().ctx().contains<SceneResourceManager*>())
            resMgr = scene->GetRegistry().ctx().get<SceneResourceManager*>();

        m_HierarchySystem.Update(scene);
        if (resMgr)
            resMgr->OnUpdate(scene, ts);
        if (m_PhysicsSystem) m_PhysicsSystem->Update(scene, ts, true);
    }

    void OnRuntimeStart(Scene* scene) override
    {
        m_HierarchySystem.Update(scene);
    }

    void OnUpdateEditor(Scene* scene, Timestep ts) override
    {
        SceneResourceManager* resMgr = nullptr;
        if (scene->GetRegistry().ctx().contains<SceneResourceManager*>())
            resMgr = scene->GetRegistry().ctx().get<SceneResourceManager*>();

        m_HierarchySystem.Update(scene);
        if (resMgr)
            resMgr->OnUpdateEditor(scene, ts);
        if (m_PhysicsSystem) m_PhysicsSystem->Update(scene, ts, false);
    }

    void OnRuntimeStop(Scene* scene) override
    {
        SceneResourceManager* resMgr = nullptr;
        if (scene->GetRegistry().ctx().contains<SceneResourceManager*>())
            resMgr = scene->GetRegistry().ctx().get<SceneResourceManager*>();
        if (resMgr)
            resMgr->OnRuntimeStop(scene);
        Physics::ClearContext(scene);
    }

private:
    HierarchySystem m_HierarchySystem;
    // Animation and audio are handled by SceneResourceManager now.
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    PhysicsSystem* m_PhysicsSystem = nullptr;
};
} // namespace CHEngine

#endif // CH_SCENE_SYSTEMS_IMPL_H
