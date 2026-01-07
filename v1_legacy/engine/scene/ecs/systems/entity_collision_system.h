#ifndef CD_SCENE_ECS_SYSTEMS_ENTITY_COLLISION_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_ENTITY_COLLISION_SYSTEM_H

#include <entt/entt.hpp>

namespace CHEngine
{
/**
 * @brief System for handling AABB collisions between entities.
 */
class EntityCollisionSystem
{
public:
    static void Update(entt::registry &registry, float deltaTime);
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_ENTITY_COLLISION_SYSTEM_H
