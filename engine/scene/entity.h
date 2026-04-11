#ifndef CH_ENTITY_H
#define CH_ENTITY_H

#include "engine/core/ch_assert.h"
#include "engine/core/base.h"
#include "engine/core/uuid.h"
#include "engine/scene/components/id_component.h"
#include "engine/scene/components/tag_component.h"
#include "entt/entt.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace CHEngine
{

struct EntityUUIDMap
{
    std::unordered_map<UUID, entt::entity> Map;
};

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, std::shared_ptr<entt::registry> registry)
        : m_EntityHandle(handle),
          m_Registry(registry)
    {
    }
    Entity(entt::entity handle, entt::registry& registry);
    Entity(entt::entity handle, entt::registry* registry);
    Entity(const Entity& other) = default;

    /** Adds a component if it is not already present and returns the inserted component. */
    template <typename T, typename... Args> T& AddOrReplaceComponent(Args&&... args)
    {
        return m_Registry->emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    /** Adds a component and asserts that the entity does not already own it. */
    template <typename T, typename... Args> T& AddComponent(Args&&... args)
    {
        CH_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
        return m_Registry->emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    /** Returns the requested component and asserts if it is missing. */
    template <typename T> T& GetComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return m_Registry->get<T>(m_EntityHandle);
    }

    /** Returns true when the entity currently owns the requested component. */
    template <typename T> bool HasComponent()
    {
        return m_Registry && m_Registry->all_of<T>(m_EntityHandle);
    }

    /** Removes the requested component and asserts if it is missing. */
    template <typename T> void RemoveComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        m_Registry->remove<T>(m_EntityHandle);
    }

    /** Applies a patch operation to the component stored on this entity. */
    template <typename T, typename... Func> void Patch(Func&&... func)
    {
        m_Registry->patch<T>(m_EntityHandle, std::forward<Func>(func)...);
    }

    operator bool() const
    {
        return m_EntityHandle != entt::null && m_Registry != nullptr;
    }
    bool IsValid() const;

    // Entity Management (Factory & Queries)
    /** Creates a child or sibling entity with the requested name in the current scene. */
    Entity Create(const std::string& name);
    /** Creates an entity with a stable UUID for serialization and duplication. */
    Entity CreateWithUUID(UUID uuid, const std::string& name);
    /** Creates a UI entity using the UI-specific component setup. */
    Entity CreateUI(const std::string& type, const std::string& name);
    /** Copies an existing entity and its components into a new entity. */
    Entity Copy(entt::entity copyEntity, entt::entity parentEntity = entt::null);
    /** Destroys the entity and removes it from the registry. */
    void Destroy();

    /** Finds the first entity with a matching TagComponent. */
    Entity FindByTag(const std::string& tag);
    /** Looks up an entity by UUID in the scene registry. */
    Entity GetByUUID(UUID uuid);

    operator entt::entity() const
    {
        return m_EntityHandle;
    }
    operator uint32_t() const
    {
        return (uint32_t)m_EntityHandle;
    }

    bool operator==(const Entity& other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Registry == other.m_Registry;
    }
    bool operator!=(const Entity& other) const
    {
        return !(*this == other);
    }

    entt::registry& GetRegistry()
    {
        return *m_Registry;
    }
    const entt::registry& GetRegistry() const
    {
        return *m_Registry;
    }

    std::shared_ptr<entt::registry> GetRegistryPtr()
    {
        return m_Registry;
    }

    UUID GetUUID()
    {
        return GetComponent<IDComponent>().ID;
    }

    const std::string& GetName()
    {
        return GetComponent<TagComponent>().Tag;
    }

private:
    entt::entity m_EntityHandle{entt::null};
    std::shared_ptr<entt::registry> m_Registry = nullptr;
};
} // namespace CHEngine

#endif // CH_ENTITY_H
