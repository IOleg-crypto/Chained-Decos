
#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include <Collision/CollisionManager.h>
#include <World/Physics.h>
#include <iostream>
#include <raylib.h>
#include <raymath.h>


// Forward declarations
class Player;

// PlayerMovement: handles all movement-related functionality
class PlayerMovement
{
public:
    PlayerMovement(Player *player);
    ~PlayerMovement() = default;

    // Movement methods
    void Move(const Vector3 &moveVector);
    void SetPosition(const Vector3 &pos);
    Vector3 GetPosition() const;
    void ApplyJumpImpulse(float impulse);

    // Physics-related methods
    void ApplyGravity(float deltaTime);
    void ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime);
    void ApplyAirborneMovement(const Vector3 &worldMoveDir, float deltaTime);
    Vector3 StepMovement(const CollisionManager &collisionManager);
    void SnapToGroundIfNeeded(const CollisionManager &collisionManager);
    void ResolveCollision(const Vector3 &response);
    Vector3 ClampMovementPerFrame(const Vector3 &movement, float maxMove);
    bool TryStepUp(const Vector3 &targetPos, const Vector3 &response);
    void WallSlide(const Vector3 &currentPos, const Vector3 &movement, const Vector3 &response);
    bool ExtractFromCollider();

    // Getters/Setters
    float GetRotationY() const;
    float GetSpeed();
    void SetSpeed(float speed);
    PhysicsComponent &GetPhysics();
    const PhysicsComponent &GetPhysics() const;

    // Set reference to collision manager
    void SetCollisionManager(const CollisionManager *collisionManager);

private:
    Player *m_player;
    Vector3 m_position;
    Vector3 m_lastPosition;
    float m_rotationY = 0.0f;

    float m_walkSpeed = 3.0f;
    float m_runSpeed = 15.0f;
    PhysicsComponent m_physics;

    // Stuck detection
    int m_stuckCounter = 0;
    float m_lastStuckTime = 0.0f;

    // Reference to the engine's collision manager
    const CollisionManager *m_lastCollisionManager = nullptr;
};

#endif // PLAYER_MOVEMENT_H
