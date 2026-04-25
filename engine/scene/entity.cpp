#include "engine/scene/entity.h"
#include "engine/core/uuid.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/components.h"
#include <unordered_map>

namespace CHEngine
{

Entity::Entity(entt::entity handle, entt::registry& registry)
    : m_EntityHandle(handle)
{
    auto* weakRegistryPtr = registry.ctx().find<std::weak_ptr<entt::registry>>();
    if (weakRegistryPtr)
    {
        m_Registry = weakRegistryPtr->lock();
    }
    else
    {
        // Fallback: Use a non-owning shared_ptr if we're initialized from a raw registry
        // that doesn't have a shared context (e.g. in stack-allocated test registries).
        m_Registry = std::shared_ptr<entt::registry>(&registry, [](entt::registry*) {});
    }
    CH_CORE_ASSERT(m_Registry, "Entity initialized with null registry!");
}

Entity::Entity(entt::entity handle, entt::registry* registry)
    : m_EntityHandle(handle)
{
    if (registry)
    {
        auto* weakRegistryPtr = registry->ctx().find<std::weak_ptr<entt::registry>>();
        if (weakRegistryPtr)
        {
            m_Registry = weakRegistryPtr->lock();
        }
        else
        {
            // Fallback: Use a non-owning shared_ptr
            m_Registry = std::shared_ptr<entt::registry>(registry, [](entt::registry*) {});
        }
    }
    CH_CORE_ASSERT(m_Registry, "Entity initialized with null registry!");
}

bool Entity::IsValid() const
{
    return m_EntityHandle != entt::null && m_Registry != nullptr && m_Registry->valid(m_EntityHandle);
}

void Entity::Destroy()
{
    if (!IsValid())
    {
        return;
    }

    std::vector<entt::entity> entitiesToDestroy;
    entitiesToDestroy.push_back(m_EntityHandle);

    size_t current = 0;
    while (current < entitiesToDestroy.size())
    {
        Entity e(entitiesToDestroy[current++], m_Registry);
        if (e.HasComponent<HierarchyComponent>())
        {
            auto& hc = e.GetComponent<HierarchyComponent>();
            for (auto child : hc.Children)
            {
                if (std::find(entitiesToDestroy.begin(), entitiesToDestroy.end(), child) == entitiesToDestroy.end())
                {
                    entitiesToDestroy.push_back(child);
                }
            }
        }
    }

    for (auto it = entitiesToDestroy.rbegin(); it != entitiesToDestroy.rend(); ++it)
    {
        if (m_Registry->valid(*it))
        {
            m_Registry->destroy(*it);
        }
    }
}

} // namespace CHEngine
