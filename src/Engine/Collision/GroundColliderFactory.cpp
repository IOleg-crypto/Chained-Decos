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
    // Ground positioned to align visual model with collision
    Vector3 groundCenter = {0.0f, 0.0f, 0.0f};
    Vector3 groundSize = {1000.0f, 1.0f, 1000.0f};

    return CreateAabbGround(groundCenter, groundSize);
}
