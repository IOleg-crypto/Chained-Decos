#include "GroundColliderFactory.h"
#include "../Physics/PhysicsComponent.h"

Collision GroundColliderFactory::CreateAabbGround(const Vector3& center, const Vector3& halfSize)
{
    Collision ground(center, halfSize);
    ground.SetCollisionType(CollisionType::AABB_ONLY);
    return ground;
}

Collision GroundColliderFactory::CreateDefaultGameGround()
{
    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
    
    float halfX = groundSize.x * 50.0f;
    float halfZ = groundSize.z * 1.75f;
    Vector3 halfSize = {halfX, groundSize.y * 0.5f, halfZ};
    
    return CreateAabbGround(groundCenter, halfSize);
}
