#ifndef PLAYER_MOVEMENT_H
#define PLAYER_MOVEMENT_H

#include <servers/physics/collision/Core/CollisionManager.h>
#include <scene/main/Core/World.h>
#include <raylib.h>
#include <raymath.h>
#include <servers/physics/dynamics/Components/PhysicsComponent.h>
#include "../Interfaces/IPlayerMovement.h"
#include "../Interfaces/IPlayerMediator.h"

class PlayerMovement : public IPlayerMovement
{
public:
    explicit PlayerMovement(IPlayerMediator *player);
    ~PlayerMovement() = default;

    // IPlayerMovement interface implementation
    void Move(const Vector3 &moveVector) override;
    void SetPosition(const Vector3 &pos) override;
    Vector3 GetPosition() const override;
    void ApplyJumpImpulse(float impulse) override;

    // Physics & collision
    void ApplyGravity(float deltaTime) override;
    Vector3 StepMovement(const CollisionManager &collisionManager) override;
    void SnapToGround(const CollisionManager &collisionManager) override;
    void UpdateGrounded(const CollisionManager &collisionManager) override;
    void HandleCollisionVelocity(const Vector3 &responseNormal) override;
    bool ExtractFromCollider() override;
    Vector3 ValidateCollisionResponse(const Vector3 &response, const Vector3 &currentPosition) override;

    // Getters/Setters
    float GetRotationY() const override;
    void SetRotationY(float rotation) override;
    float GetSpeed() const override;
    void SetSpeed(float speed) override;
    
    PhysicsComponent &GetPhysics() override;
    const PhysicsComponent &GetPhysics() const override;

    // Noclip functionality
    void SetNoclip(bool enable) override;
    bool IsNoclip() const override;

    // Reference to collision manager
    void SetCollisionManager(const CollisionManager *collisionManager) override;

private:
    IPlayerMediator *m_player;
    Vector3 m_position;
    float m_rotationY = 0.0f;

    float m_walkSpeed = 11.0f;
    PhysicsComponent m_physics;

    const CollisionManager *m_lastCollisionManager = nullptr;

    // Grounding helpers
    int m_framesSinceGround = 0;
    int m_coyoteFramesRemaining = 0;

    // Noclip mode
    bool m_noclip = false;

    // Constants
    static constexpr int GROUNDED_SET_FRAMES = 2;
    static constexpr int GROUNDED_CLEAR_FRAMES = 3;
    static constexpr int COYOTE_FRAMES = 4;
    static constexpr float MAX_FALL_SPEED = -20.0f;
    static constexpr float SKIN_WIDTH = 0.001f;
};

#endif // PLAYER_MOVEMENT_H