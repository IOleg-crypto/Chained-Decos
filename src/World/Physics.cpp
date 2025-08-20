#include <World/Physics.h>
#include <Player/Player.h>
#include <iostream>

void PhysicsComponent::Update(float dt) { m_dt = dt; }

void PhysicsComponent::ApplyGravity(Vector3 &position)
{
    if (!m_isGrounded)
    {
        m_velocityY -= m_gravity * m_dt;
        position.y += m_velocityY * m_dt;
    }
}

void PhysicsComponent::TryJump()
{
    if (m_isGrounded)
    {
        m_velocityY = m_jumpStrength;
        m_isGrounded = false;
        m_isJumping = true;
    }
}

void PhysicsComponent::Land()
{
    m_velocityY = 0.0f;
    m_isGrounded = true;
    m_isJumping = false;
}

void PhysicsComponent::CancelVerticalVelocity() { m_velocityY = 0.0f; }

void PhysicsComponent::SetInAir() { m_isGrounded = false; }

bool PhysicsComponent::IsGrounded() const { return m_isGrounded; }

void PhysicsComponent::SetJumpStrength(float strength) { m_jumpStrength = strength; }

void PhysicsComponent::SetGravity(float g) { m_gravity = g; }

float PhysicsComponent::GetVelocityY() const { return m_velocityY; }

float PhysicsComponent::GetGravity() const { return m_gravity; }

float PhysicsComponent::GetJumpStrength() const { return m_jumpStrength; }

void PhysicsComponent::SetVelocityY(float velocity) { this->m_velocityY = velocity; }

bool PhysicsComponent::IsJumping() const { return m_isJumping; }

Vector3 PhysicsComponent::GetVelocity() const { return m_velocity; }

void PhysicsComponent::SetGroundLevel(bool isGrounded) { this->m_isGrounded = isGrounded; }

void PhysicsComponent::SetVelocity(const Vector3 &velocity) { this->m_velocity = velocity; }

void PhysicsComponent::AddVelocity(const Vector3 &delta)
{
    m_velocity = Vector3Add(m_velocity, delta);
}

void PhysicsComponent::SetJumpState(bool setJumpState) {
    m_isJumping = setJumpState;
}

float PhysicsComponent::GetDeltaTime() const { return m_dt; }

PhysicsComponent::PhysicsComponent() : m_dt(GetFrameTime()) {}

