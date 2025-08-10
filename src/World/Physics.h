#ifndef PHYSICS_H
#define PHYSICS_H

#include <raylib.h>
#include <raymath.h>
#include <Collision/CollisionManager.h>

//
// PhysicsComponent
// Handles gravity, jumping, velocity, and grounded state for game entities.
// This class is purely responsible for motion physics, not rendering or input.
//
class PhysicsComponent
{
public:
    PhysicsComponent();

    // -------------------- Update & Core Physics --------------------

    // Update physics state (gravity, velocity, etc.)
    // dt - Delta time in seconds
    void Update(float dt);

    // Apply gravity to the given position
    void ApplyGravity(Vector3 &position);

    // Attempt to start a jump if grounded
    void TryJump();

    // Mark the object as landed (grounded)
    void Land();

    // Immediately stop vertical motion
    void CancelVerticalVelocity();

    // Mark object as in-air (falling/jumping)
    void SetInAir();

    // -------------------- Configuration --------------------

    // Set jump impulse strength
    void SetJumpStrength(float strength);

    // Set gravity acceleration
    void SetGravity(float g);

    // Directly set vertical velocity
    void SetVelocityY(float velocity);

    // Set grounded state
    void SetGroundLevel(bool isGrounded);

    // Set full velocity vector
    void SetVelocity(const Vector3 &velocity);

    // Add to current velocity
    void AddVelocity(const Vector3 &delta);

    // -------------------- Getters --------------------

    // Returns true if the object is touching the ground
    [[nodiscard]] bool IsGrounded() const;

    // Returns current vertical velocity
    [[nodiscard]] float GetVelocityY() const;

    // Returns gravity acceleration
    [[nodiscard]] float GetGravity() const;

    // Returns jump strength
    [[nodiscard]] float GetJumpStrength() const;

    // Returns last delta time used for physics updates
    [[nodiscard]] float GetDeltaTime() const;

    // Returns true if currently jumping
    [[nodiscard]] bool IsJumping() const;

    // Returns full velocity vector
    [[nodiscard]] Vector3 GetVelocity() const;

private:
    // Physics parameters
    float m_gravity = 10.0f;        // Gravity acceleration
    float m_velocityY = 0.0f;       // Vertical velocity
    bool m_isGrounded = true;       // Whether object is on the ground
    float m_groundLevel = 8.0f;     // Ground height level
    float m_dt = 0.0f;              // Delta time (cached from last update)
    float m_jumpStrength = 8.0f;    // Jump impulse strength
    bool m_isJumping = false;       // Jump state
    Vector3 m_velocity = {0, 0, 0}; // Full velocity vector
};

#endif // PHYSICS_H
