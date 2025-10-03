#include "GroundColliderFactory.h"
#include "Physics/PhysicsComponent.h"

Collision GroundColliderFactory::CreateAabbGround(const Vector3& center, const Vector3& halfSize)
{
    Collision ground(center, halfSize);
    ground.SetCollisionType(CollisionType::AABB_ONLY);
    return ground;
}

Collision GroundColliderFactory::CreateDefaultGameGround()
{
    // Fix ground positioning - place it at the world floor level
    Vector3 groundCenter = {0.0f, 0.0f, 0.0f};
    Vector3 groundSize = {1000.0f, 0.0f, 1000.0f};

    float halfX = groundSize.x * 0.5f;
    float halfZ = groundSize.z * 0.5f;
    Vector3 halfSize = {halfX, groundSize.y * 0.5f, halfZ};

    return CreateAabbGround(groundCenter, halfSize);
}
