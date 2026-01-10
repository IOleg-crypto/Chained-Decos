#include "scene.h"
#include "core/events.h"
#include "core/log.h"
#include "engine/core/events.h"
#include "engine/scene/core/scene_manager.h"
#include "engine/scene/ecs/components/core/id_component.h"
#include "engine/scene/ecs/components/core/tag_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include "engine/scene/ecs/systems/physics_system.h"
#include "entity.h"


namespace CHEngine
{

// =========================================================================
// Getters & Setters
// =========================================================================

entt::registry &Scene::GetRegistry()
{
    return m_Registry;
}

const entt::registry &Scene::GetRegistry() const
{
    return m_Registry;
}

const std::string &Scene::GetName() const
{
    return m_Name;
}

void Scene::SetName(const std::string &name)
{
    m_Name = name;
}

uint32_t Scene::GetViewportWidth() const
{
    return m_ViewportWidth;
}

uint32_t Scene::GetViewportHeight() const
{
    return m_ViewportHeight;
}

// =========================================================================
// Lifecycle
// =========================================================================

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
    CH::MouseButtonEvent event;
    event.m_Button = MOUSE_BUTTON_LEFT;
    event.m_EventType = CH::MouseButtonEvent::EventType::Pressed;
    event.m_EventType = CH::MouseButtonEvent::EventType::Released;
    PhysicsSystem::Update(m_Registry, deltaTime);
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
