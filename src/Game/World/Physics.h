#ifndef PHYSICS_H
#define PHYSICS_H

#include <Collision/CollisionManager.h>
#include <raylib.h>
#include <raymath.h>

//
// PhysicsComponent
// Handles gravity, jumping, velocity, and grounded state for game entities.
// This class is purely responsible for motion physics, not rendering or input.
//
class PhysicsComponent
{
public:
    // World physics constants
    static constexpr float WORLD_FLOOR_Y = -10.0f; // Match ground plane bottom
    static constexpr Vector2 GROUND_SIZE = {2000.0f,
                                            2000.0f}; // Increased from 800x800 to 2000x2000
    static constexpr Vector3 GROUND_POSITION = {0.0f, 1.0f, 0.0f}; // Match ground plane center
    static constexpr Vector3 GROUND_COLLISION_CENTER = {0, 1, 0};  // Match ground plane center
    static constexpr Vector3 GROUND_COLLISION_SIZE = {
        2000, 2, 2000}; // Increased from 1000x1000 to 2000x2000
    static constexpr Vector3 DEBUG_CUBE_POSITION = {0, 5, 0};
    static constexpr Vector3 DEBUG_CUBE_SIZE = {2, 2, 2};

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
    // Change player jump state
    void SetJumpState(bool setJumpState);
    // If player has extreme velocity
    bool HasExtremeVelocity(const Vector3 &vel) const;

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
    float m_gravity = 10.0f;        // Increased gravity for more responsive movement
    float m_velocityY = 0.0f;       // Vertical velocity
    bool m_isGrounded = false;      // Whether object is on the ground
    float m_groundLevel = 0.0f;     // Ground height level (top of ground plane)
    float m_dt = 0.0f;              // Delta time (cached from last update)
    float m_jumpStrength = 10.0f;   // Increased jump impulse strength
    bool m_isJumping = false;       // Jump state
    Vector3 m_velocity = {0, 0, 0}; // Full velocity vector
};

#endif // PHYSICS_H
