#ifndef IPLAYERMOVEMENT_H
#define IPLAYERMOVEMENT_H

#include <Collision/CollisionManager.h>
#include <Physics/PhysicsComponent.h>
#include <raylib.h>
#include <raymath.h>

// Forward declaration to break circular dependency
class Player;

/**
 * Interface for player movement and physics
 * Allows Player to work with different movement implementations
 */
class IPlayerMovement
{
public:
    virtual ~IPlayerMovement() = default;

    // Movement methods
    virtual void Move(const Vector3 &moveVector) = 0;
    virtual void SetPosition(const Vector3 &pos) = 0;
    virtual Vector3 GetPosition() const = 0;
    virtual void ApplyJumpImpulse(float impulse) = 0;

    // Physics & collision
    virtual void ApplyGravity(float deltaTime) = 0;
    virtual Vector3 StepMovement(const CollisionManager &collisionManager) = 0;
    virtual void SnapToGround(const CollisionManager &collisionManager) = 0;
    virtual void UpdateGrounded(const CollisionManager &collisionManager) = 0;
    virtual void HandleCollisionVelocity(const Vector3 &responseNormal) = 0;
    virtual bool ExtractFromCollider() = 0;
    virtual Vector3 ValidateCollisionResponse(const Vector3 &response, const Vector3 &currentPosition) = 0;

    // Getters/Setters
    virtual float GetRotationY() const = 0;
    virtual void SetRotationY(float rotation) = 0;
    virtual float GetSpeed() const = 0;
    virtual void SetSpeed(float speed) = 0;
    
    // Physics component access
    virtual PhysicsComponent &GetPhysics() = 0;
    virtual const PhysicsComponent &GetPhysics() const = 0;

    // Noclip functionality
    virtual void SetNoclip(bool enable) = 0;
    virtual bool IsNoclip() const = 0;

    // Collision manager reference
    virtual void SetCollisionManager(const CollisionManager *collisionManager) = 0;
};

#endif // IPLAYERMOVEMENT_H

