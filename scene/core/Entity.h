#ifndef CHENGINE_CORE_ENTITY_H
#define CHENGINE_CORE_ENTITY_H

#include "Scene.h"
#include "core/Log.h"
#include <cstdint>
#include <entt/entt.hpp>

namespace CHEngine
{

/**
 * @brief Entity - Lightweight wrapper around entt::entity
 *
 * Provides convenient API for component operations.
 * Holds reference to parent Scene for validation.
 */
class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene *scene);
    Entity(const Entity &other) = default;
    ~Entity() = default;

    // --- Component Operations ---
public:
    template <typename T, typename... Args> T &AddComponent(Args &&...args)
    {
        if (HasComponent<T>())
        {
            CD_CORE_WARN("[Entity] Entity already has component!");
            return GetComponent<T>();
        }

        return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
    }

    template <typename T> T &GetComponent()
    {
        if (!HasComponent<T>())
        {
            CD_CORE_ERROR("[Entity] Entity does not have component!");
        }

        return m_Scene->GetRegistry().get<T>(m_EntityHandle);
    }

    template <typename T> bool HasComponent()
    {
        return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
    }

    template <typename T> void RemoveComponent()
    {
        if (!HasComponent<T>())
        {
            CD_CORE_WARN("[Entity] Entity does not have component to remove!");
            return;
        }

        m_Scene->GetRegistry().remove<T>(m_EntityHandle);
    }

    // --- Operators ---
public:
    operator bool() const
    {
        return m_EntityHandle != entt::null && m_Scene != nullptr;
    }
    operator entt::entity() const
    {
        return m_EntityHandle;
    }
    operator uint32_t() const
    {
        return static_cast<uint32_t>(m_EntityHandle);
    }

    bool operator==(const Entity &other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }

    bool operator!=(const Entity &other) const
    {
        return !(*this == other);
    }

    // --- Member Variables ---
private:
    entt::entity m_EntityHandle{entt::null};
    Scene *m_Scene = nullptr;

    friend class Scene;
};

} // namespace CHEngine

#endif // ENTITY_H
