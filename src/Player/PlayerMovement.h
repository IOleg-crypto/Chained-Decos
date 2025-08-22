
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
    [[nodiscard]] Vector3 GetPosition() const;
    void ApplyJumpImpulse(float impulse);

    // Physics-related methods
    void ApplyGravity(float deltaTime);
    Vector3 StepMovement(const CollisionManager &collisionManager);
    void SnapToGroundIfNeeded(const CollisionManager &collisionManager);
    void HandleCollisionVelocity(const Vector3 &responseNormal);
    bool ExtractFromCollider();

    // Getters/Setters
    [[nodiscard]] float GetRotationY() const;
    float GetSpeed();
    void SetSpeed(float speed);
    void SetRotationY(float rotation);
    PhysicsComponent &GetPhysics();
    [[nodiscard]] const PhysicsComponent &GetPhysics() const;

    // Set reference to collision manager
    void SetCollisionManager(const CollisionManager *collisionManager);

private:
    Player *m_player;
    Vector3 m_position;
    float m_rotationY = 0.0f;

    float m_walkSpeed = 8.0f;
    PhysicsComponent m_physics;

    // Reference to the engine's collision manager
    const CollisionManager *m_lastCollisionManager = nullptr;
};

#endif // PLAYER_MOVEMENT_H
