#ifndef CH_SCENE_H
#define CH_SCENE_H

#include "components.h"
#include "engine/core/base.h"
#include "entity.h"
#include <entt/entt.hpp>
#include <string>

#include "engine/core/events.h"

namespace CHEngine
{

class Scene
{
public:
    Scene();
    ~Scene() = default;

    Entity CreateEntity(const std::string &name = "Entity");
    Entity CreateEntityWithUUID(UUID uuid, const std::string &name = "Entity");
    void DestroyEntity(Entity entity);

    Entity FindEntityByTag(const std::string &tag);
    Entity GetEntityByUUID(UUID uuid);

    void OnUpdateRuntime(float deltaTime);
    void OnUpdateEditor(float deltaTime);
    void OnEvent(Event &e);

    entt::registry &GetRegistry()
    {
        return m_Registry;
    }
    const entt::registry &GetRegistry() const
    {
        return m_Registry;
    }

    struct SkyboxComponent &GetSkybox()
    {
        return m_Skybox;
    }
    const struct SkyboxComponent &GetSkybox() const
    {
        return m_Skybox;
    }

private:
    entt::registry m_Registry;
    struct SkyboxComponent m_Skybox;

    friend class Entity;
};

// Entity Template Implementations
template <typename T, typename... Args> T &Entity::AddComponent(Args &&...args)
{
    return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
}

inline bool Entity::IsValid() const
{
    return m_Scene && m_Scene->m_Registry.valid(m_EntityHandle);
}

template <typename T> T &Entity::GetComponent()
{
    return m_Scene->m_Registry.get<T>(m_EntityHandle);
}

template <typename T> bool Entity::HasComponent()
{
    return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
}

template <typename T> void Entity::RemoveComponent()
{
    m_Scene->m_Registry.remove<T>(m_EntityHandle);
}
} // namespace CHEngine

#endif // CH_SCENE_H
