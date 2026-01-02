#ifndef CD_SCENE_ECS_SYSTEMS_LIFETIME_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_LIFETIME_SYSTEM_H

#include <entt/entt.hpp>

namespace CHEngine
{
/**
 * @brief System for handling entity lifetimes.
 * Automatically destroys entities when their LifetimeComponent timer exceeds lifetime.
 */
class LifetimeSystem
{
public:
    static void Update(entt::registry &registry, float deltaTime);
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_LIFETIME_SYSTEM_H
