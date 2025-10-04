#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include <memory>
#include <vector>
#include <raylib.h>
#include <raymath.h>
#include "SurfaceComponent.h"



class PhysicsComponent {
public:
    // World physics constants
    static constexpr float WORLD_FLOOR_Y = -1.0f;

    PhysicsComponent();

    void Update(float deltaTime);

    // Physics state
    bool IsGrounded() const;
    void SetGroundLevel(bool isGrounded);

    bool IsKinematic() const;
    void SetKinematic(bool kinematic);

    bool IsJumping() const;
    void SetJumpState(bool jumping);

    // Forces and movement
    Vector3 GetVelocity() const;
    void SetVelocity(const Vector3 &velocity);
    void AddVelocity(const Vector3 &delta);

    float GetVelocityY() const;
    void SetVelocityY(float y);
    void CancelVerticalVelocity();

    // Physics properties
    float GetGravity() const;
    void SetGravity(float gravity);

    float GetJumpStrength() const;
    void SetJumpStrength(float strength);

    float GetDrag() const;
    void SetDrag(float drag);

    float GetDeltaTime() const;

    // Actions
    void TryJump();
    void Land();
    void SetInAir();

    // Surface interaction
    void HandleSurfaceInteraction(const SurfaceComponent* surface);

    // Utility
    bool HasExtremeVelocity(const Vector3& velocity) const;

    bool HasExtremeVelocity() const;


private:
    // State
    bool m_isGrounded = false;
    bool m_isKinematic = false;
    bool m_isJumping = false;
    float m_dt = 0.0f;

    // Motion
    Vector3 m_velocity{0};
    Vector3 m_forces{0};

    // Properties
    float m_gravity = 9.81f;
    float m_jumpStrength = 10.0f;
    float m_drag = 0.1f;
    static constexpr float MAX_SPEED = 300.0f;

    // Physics simulation
    void ApplyPhysics(float deltaTime);
    void ApplyGravity(float deltaTime);
    void ApplyDrag(float deltaTime);
    void IntegrateForces(float deltaTime);
};
#endif
