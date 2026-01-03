#ifndef CD_SCENE_ECS_ECS_REGISTRY_H
#define CD_SCENE_ECS_ECS_REGISTRY_H

#include <entt/entt.hpp>

// Global registry for the entire project
#include "engine/scene/core/scene_manager.h"

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
#define CD_SCENE_ECS_ECS_REGISTRY_H

#endif // CD_SCENE_ECS_ECS_REGISTRY_H
