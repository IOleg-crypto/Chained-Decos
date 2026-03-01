#ifndef CH_ENTITY_H
#define CH_ENTITY_H

#include "engine/core/assert.h"
#include "engine/core/base.h"
#include "engine/scene/components/id_component.h"
#include "engine/scene/components/tag_component.h"
#include "entt/entt.hpp"

#include <string>

namespace CHEngine
{
class Scene;

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, entt::registry* registry)
        : m_EntityHandle(handle),
          m_Registry(registry)
    {
    }
    Entity(const Entity& other) = default;

    template <typename T, typename... Args> T& AddOrReplaceComponent(Args&&... args)
    {
        return m_Registry->emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> T& AddComponent(Args&&... args)
    {
        CH_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
        return m_Registry->emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template <typename T> T& GetComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        return m_Registry->get<T>(m_EntityHandle);
    }

    template <typename T> bool HasComponent()
    {
        return m_Registry && m_Registry->all_of<T>(m_EntityHandle);
    }

    template <typename T> void RemoveComponent()
    {
        CH_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
        m_Registry->remove<T>(m_EntityHandle);
    }

    template <typename T, typename... Func> void Patch(Func&&... func)
    {
        m_Registry->patch<T>(m_EntityHandle, std::forward<Func>(func)...);
    }

    operator bool() const
    {
        return m_EntityHandle != entt::null && m_Registry != nullptr;
    }
    bool IsValid() const;
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
    entt::registry* m_Registry = nullptr;
};
} // namespace CHEngine

#endif // CH_ENTITY_H
