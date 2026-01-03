#ifndef CD_SCENE_ECS_COMPONENTS_UTILITY_COMPONENTS_H
#define CD_SCENE_ECS_COMPONENTS_UTILITY_COMPONENTS_H

#include <string>
namespace CHEngine
{
struct LifetimeComponent
{
    float lifetime = 5.0f; // Lifetime in seconds
    float timer = 0.0f;    // Current time
    bool destroyOnTimeout = true;
};

struct NameComponent
{
    std::string name; // Entity name for debugging
};

struct MapObjectIndex
{
    int index; // Index in GameScene.GetMapObjects()
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_UTILITY_COMPONENTS_H
