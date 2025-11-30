#ifndef ECS_REGISTRY_H
#define ECS_REGISTRY_H

#include <entt/entt.hpp>

// Глобальний registry для всього проєкту
class ECSRegistry
{
    static entt::registry s_registry;

public:
    static entt::registry &Get()
    {
        return s_registry;
    }

    // Helper методи
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

// Зручний макрос
#define REGISTRY ECSRegistry::Get()

#endif // ECS_REGISTRY_H
