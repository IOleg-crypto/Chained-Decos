#ifndef CD_SCENE_ECS_COMPONENTS_ENVIRONMENT_COMPONENTS_H
#define CD_SCENE_ECS_COMPONENTS_ENVIRONMENT_COMPONENTS_H

#include <raylib.h>
#include <string>

namespace CHEngine
{

// Component for ground plane entities
// Replaces WorldManager ground segments
struct GroundPlaneComponent
{
    Vector2 size = {100.0f, 100.0f};
    float height = 0.0f;
    Color color = DARKGRAY;
    bool renderGrid = true;
    float gridSpacing = 1.0f;
};

// Component for world boundary definition
// Replaces WorldManager world bounds
struct WorldBoundsComponent
{
    BoundingBox bounds = {{-1000, -100, -1000}, {1000, 100, 1000}};
    bool drawDebug = false;
    Color debugColor = GREEN;
};

// Component marking an entity as part of the environment
// Used for hierarchy organization
struct EnvironmentTag
{
    std::string category = "Environment"; // Environment, Lighting, etc.
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_ENVIRONMENT_COMPONENTS_H
