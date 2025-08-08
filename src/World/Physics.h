#ifndef PHYSICS_H
#define PHYSICS_H

#include <raylib.h>
#include <raymath.h>

class PhysicsComponent
{
  public:
    PhysicsComponent();

  public:
    void Update(float dt);
    void ApplyGravity(Vector3 &position);
    void TryJump();
    void Land();
    void CancelVerticalVelocity();
    void SetInAir();

    // Config
    void SetJumpStrength(float strength);
    void SetGravity(float g);
    void SetVelocityY(float velocity);
    void SetGroundLevel(bool isGrounded);
    void SetVelocity(const Vector3 &velocity);
    void AddVelocity(const Vector3 &delta);

    // Getters
    [[nodiscard]] bool IsGrounded() const;
    [[nodiscard]] float GetVelocityY() const;
    [[nodiscard]] float GetGravity() const;
    [[nodiscard]] float GetJumpStrength() const;
    [[nodiscard]] float GetDeltaTime() const;
    [[nodiscard]] bool IsJumping() const;
    [[nodiscard]] Vector3 GetVelocity() const;

  private:
    float m_gravity = 10.0f;
    float m_velocityY = 0.0f;
    bool m_isGrounded = true;
    float m_groundLevel = 8.0f;
    float m_dt = 0.0f;
    float m_jumpStrength = 8.0f;
    bool m_isJumping = false;
    Vector3 m_velocity = {0, 0, 0};
};

#endif // PHYSICS_H