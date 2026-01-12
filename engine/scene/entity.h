#ifndef CH_ENTITY_H
#define CH_ENTITY_H

#include <entt/entt.hpp>

namespace CHEngine
{
class Scene;

class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene *scene) : m_EntityHandle(handle), m_Scene(scene)
    {
    }
    Entity(const Entity &other) = default;

    template <typename T, typename... Args> T &AddComponent(Args &&...args);

    template <typename T> T &GetComponent();

    template <typename T> bool HasComponent();

    template <typename T> void RemoveComponent();

    operator bool() const
    {
        return m_EntityHandle != entt::null;
    }
    operator entt::entity() const
    {
        return m_EntityHandle;
    }
    operator uint32_t() const
    {
        return (uint32_t)m_EntityHandle;
    }

    bool operator==(const Entity &other) const
    {
        return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
    }
    bool operator!=(const Entity &other) const
    {
        return !(*this == other);
    }

    Scene *GetScene()
    {
        return m_Scene;
    }
    const Scene *GetScene() const
    {
        return m_Scene;
    }

private:
    entt::entity m_EntityHandle{entt::null};
    Scene *m_Scene = nullptr;
};
} // namespace CHEngine

#endif // CH_ENTITY_H
