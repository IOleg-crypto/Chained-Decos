#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include <entt/entt.hpp>
#include <raylib.h>


struct PhysicsComponent
{
    float mass = 1.0f;
    float gravity = -9.8f;
    bool useGravity = true;
    bool isKinematic = false; // Не реагує на фізику (контролюється вручну)

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
    bool isTrigger = false; // Не блокує рух, тільки події
    int collisionLayer = 0; // Шар об'єкта (0-31)
    int collisionMask = ~0; // З якими шарами може зіткнутися (bitmask)

    // Collision callbacks (можна розширити)
    bool hasCollision = false;
    entt::entity collidedWith = entt::null;
};

#endif // PHYSICS_COMPONENT_H
