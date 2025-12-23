#ifndef ECS_REGISTRY_WRAPPER_H
#define ECS_REGISTRY_WRAPPER_H

#include <entt/entt.hpp>

// Global registry for the entire project
#include "core/Engine.h"

// Global registry wrapper that uses the Engine's registry
class ECSRegistry
{
public:
    static entt::registry &Get()
    {
        return CHEngine::Engine::Instance().GetECSRegistry();
    }

    // Helper methods
    static entt::entity CreateEntity()
    {
        return Get().create();
    }

    static void DestroyEntity(entt::entity entity)
    {
        Get().destroy(entity);
    }

    static void Clear()
    {
        Get().clear();
    }
};

// Convenient macro
#define REGISTRY ECSRegistry::Get()

#endif // ECS_REGISTRY_WRAPPER_H
