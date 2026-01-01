#ifndef ECS_REGISTRY_WRAPPER_H
#define ECS_REGISTRY_WRAPPER_H

#include <entt/entt.hpp>

// Global registry for the entire project
#include "scene/core/SceneManager.h"

// Global registry wrapper that uses the Active Scene's registry
class ECSRegistry
{
public:
    static entt::registry &Get()
    {
        auto scene = CHEngine::SceneManager::GetActiveScene();
        if (scene)
            return scene->GetRegistry();

        static entt::registry s_Dummy;
        return s_Dummy;
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
