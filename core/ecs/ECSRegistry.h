#ifndef ECS_REGISTRY_WRAPPER_H
#define ECS_REGISTRY_WRAPPER_H

#include <entt/entt.hpp>

// Global registry for the entire project
class ECSRegistry
{
    static entt::registry s_registry;

public:
    static entt::registry &Get()
    {
        return s_registry;
    }

    // Helper methods
    static entt::entity CreateEntity()
    {
        return s_registry.create();
    }

    static void DestroyEntity(entt::entity entity)
    {
        s_registry.destroy(entity);
    }

    static void Clear()
    {
        s_registry.clear();
    }
};

// Convenient macro
#define REGISTRY ECSRegistry::Get()

#endif // ECS_REGISTRY_WRAPPER_H
