#include "scene_system_manager.h"
#include "engine/scene/scene.h"
#include "engine/core/profiler.h"

namespace CHEngine
{
    SceneSystemManager::SceneSystemManager(Scene* scene)
        : m_Scene(scene)
    {
    }

    void SceneSystemManager::OnRuntimeStart()
    {
        for (auto& system : m_Systems)
            system->OnRuntimeStart(m_Scene);
    }

    void SceneSystemManager::OnRuntimeStop()
    {
        for (auto& system : m_Systems)
            system->OnRuntimeStop(m_Scene);
    }

    void SceneSystemManager::OnUpdate(Timestep ts)
    {
        for (auto& system : m_Systems)
        {
            CH_PROFILE_SCOPE("SceneSystem::Update");
            system->OnUpdate(m_Scene, ts);
        }
    }

    void SceneSystemManager::OnUpdateEditor(Timestep ts)
    {
        for (auto& system : m_Systems)
        {
            CH_PROFILE_SCOPE("SceneSystem::UpdateEditor");
            system->OnUpdateEditor(m_Scene, ts);
        }
    }
    void SceneSystemManager::InitObservers()
    {
        for (auto& system : m_Systems)
            system->RegisterObservers(m_Scene->GetRegistry());
    }
}
