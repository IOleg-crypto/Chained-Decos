#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include <Collision/CollisionManager.h>
#include <World/World.h>
#include <raylib.h>
#include <raymath.h>
#include <Physics/PhysicsComponent.h>

// Forward declarations
class Player;

class PlayerMovement
{
public:
    explicit PlayerMovement(Player *player);
    ~PlayerMovement() = default;

    // Movement methods
    void Move(const Vector3 &moveVector);
    void SetPosition(const Vector3 &pos);
    [[nodiscard]] Vector3 GetPosition() const;
    void ApplyJumpImpulse(float impulse);

    // Physics & collision
    void ApplyGravity(float deltaTime);
    Vector3 StepMovement(const CollisionManager &collisionManager);
    void SnapToGround(const CollisionManager &collisionManager);
    
    // Unified grounded detection using raycast
    void UpdateGrounded(const CollisionManager &collisionManager);
    void HandleCollisionVelocity(const Vector3 &responseNormal);
    bool ExtractFromCollider();
    Vector3 ValidateCollisionResponse(const Vector3& response, const Vector3& currentPosition);

    // Getters/Setters
    [[nodiscard]] float GetRotationY() const;
    void SetRotationY(float rotation);

    float GetSpeed();

    float GetSpeed() const;
    void SetSpeed(float speed);
    PhysicsComponent &GetPhysics();
    [[nodiscard]] const PhysicsComponent &GetPhysics() const;

    // Reference to collision manager
    void SetCollisionManager(const CollisionManager *collisionManager);

private:
    Player *m_player;
    Vector3 m_position;
    float m_rotationY = 0.0f;

    float m_walkSpeed = 11.0f;
    PhysicsComponent m_physics;

    const CollisionManager *m_lastCollisionManager = nullptr;

    // Grounding helpers
    int m_framesSinceGround = 0;
    int m_coyoteFramesRemaining = 0;

    // Constants
    static constexpr int GROUNDED_SET_FRAMES = 2;
    static constexpr int GROUNDED_CLEAR_FRAMES = 3;
    static constexpr int COYOTE_FRAMES = 4;
    static constexpr float MAX_FALL_SPEED = -50.0f;
    static constexpr float SKIN_WIDTH = 0.001f;
};

#endif // PLAYER_MOVEMENT_H