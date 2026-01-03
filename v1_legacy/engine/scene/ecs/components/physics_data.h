#ifndef CD_SCENE_ECS_COMPONENTS_PHYSICS_DATA_H
#define CD_SCENE_ECS_COMPONENTS_PHYSICS_DATA_H

#include <entt/entt.hpp>
#include <memory>
#include <raylib.h>

class Collision;

namespace CHEngine
{

// ECS version (Data-only struct)
struct PhysicsData
{
    float mass = 1.0f;
    float gravity = -9.8f;
    bool useGravity = true;
    bool isKinematic = false; // Does not react to physics (controlled manually)

    // Friction and bounciness
    float friction = 0.5f;
    float bounciness = 0.0f; // 0 = no bounce, 1 = perfect bounce

    // Constraints
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;
    bool freezeRotation = false;
};

struct CollisionComponent
{
    BoundingBox bounds;
    bool isTrigger = false; // Does not block movement, only events
    int collisionLayer = 0; // Object layer (0-31)
    int collisionMask = ~0; // Which layers it can collide with (bitmask)

    // Collision callbacks (can be extended)
    bool hasCollision = false;
    entt::entity collidedWith = entt::null;

    // Collider pointer for collision system
    std::shared_ptr<Collision> collider = nullptr;
};

} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_PHYSICS_DATA_H
