#ifndef CD_SCENE_ECS_SYSTEMS_PLAYER_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_PLAYER_SYSTEM_H

#include <entt/entt.hpp>

namespace CHEngine
{
/**
 * @brief System for handling player input, movement, camera, and audio.
 */
class PlayerSystem
{
public:
    static void Update(entt::registry &registry, float deltaTime);
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_PLAYER_SYSTEM_H
