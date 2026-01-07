#ifndef CD_SCENE_ECS_COMPONENTS_SPAWN_POINT_COMPONENT_H
#define CD_SCENE_ECS_COMPONENTS_SPAWN_POINT_COMPONENT_H

#include <raylib.h>
#include <string>

namespace CHEngine
{

/**
 * SpawnPointComponent - Defines a location and zone where entities can spawn.
 */
struct SpawnPointComponent
{
    std::string Name = "DefaultSpawn";
    bool IsActive = true;

    // Optional zone (if zero, it's a point spawn)
    Vector3 ZoneSize = {0.0f, 0.0f, 0.0f};

    SpawnPointComponent() = default;
    SpawnPointComponent(const std::string &name) : Name(name)
    {
    }
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_SPAWN_POINT_COMPONENT_H
