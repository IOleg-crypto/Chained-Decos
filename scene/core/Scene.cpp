#include "Scene.h"
#include "Entity.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/scripting/ScriptManager.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/core/IDComponent.h"
#include "scene/ecs/components/core/TagComponent.h"

namespace CHEngine
{

Scene::Scene(const std::string &name) : m_Name(name)
{
    CD_CORE_INFO("[Scene] Created scene: %s", name.c_str());
}

Entity Scene::CreateEntity(const std::string &name)
{
    return CreateEntityWithUUID(0, name); // UUID will be generated
}

Entity Scene::CreateEntityWithUUID(uint64_t uuid, const std::string &name)
{
    Entity entity = {m_Registry.create(), this};

    // Add core components
    entity.AddComponent<IDComponent>(uuid);
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<TransformComponent>();

    CD_CORE_TRACE("[Scene] Created entity: %s (ID: %llu)", name.c_str(), uuid);
    return entity;
}

void Scene::DestroyEntity(Entity entity)
{
    DestroyEntity(entity.m_EntityHandle);
}

void Scene::DestroyEntity(entt::entity entity)
{
    if (m_Registry.all_of<TagComponent>(entity))
    {
        auto &tag = m_Registry.get<TagComponent>(entity);
        CD_CORE_TRACE("[Scene] Destroying entity: %s", tag.Tag.c_str());
    }

    m_Registry.destroy(entity);
}

void Scene::OnUpdateRuntime(float deltaTime)
{
    // 1. Update Scripts
    auto &scriptManager = Engine::Instance().GetScriptManager();
    scriptManager.SetActiveRegistry(&m_Registry);
    scriptManager.UpdateScripts(m_Registry, deltaTime);
}

void Scene::OnUpdateEditor(float deltaTime)
{
    // Editor update logic (no physics, just visual updates)
    (void)deltaTime;
}

void Scene::OnRenderRuntime()
{
    // Runtime rendering logic
}

void Scene::OnRenderEditor()
{
    // Editor rendering logic
}

void Scene::SetViewportSize(uint32_t width, uint32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
}

} // namespace CHEngine
