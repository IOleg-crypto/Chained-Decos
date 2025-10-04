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
    // Fix ground positioning - account for player's visual MODEL_Y_OFFSET
    // The player's visual model is offset by -1.0f, so we need to position
    // the ground collision 1 unit higher to make the visual model appear on the ground
    Vector3 groundCenter = {0.0f, -6.0f, 0.0f}; // Moved up by 1.0f to account for visual offset
    Vector3 groundSize = {1000.0f, -1.0f, 1000.0f};

    // float halfX = groundSize.x * 0.5f;
    // float halfZ = groundSize.z * 0.5f;
    // Vector3 halfSize = {halfX, groundSize.y * 0.5f, halfZ};

    return CreateAabbGround(groundCenter, groundSize);
}
