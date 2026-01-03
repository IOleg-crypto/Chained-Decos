#ifndef CD_SCENE_ECS_SYSTEMS_PHYSICS_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_PHYSICS_SYSTEM_H

#include <entt/entt.hpp>

namespace CHEngine
{
/**
 * @brief System for handling physics and collisions in Pure ECS.
 */
class PhysicsSystem
{
public:
    static void Update(entt::registry &registry, float deltaTime);
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_PHYSICS_SYSTEM_H
