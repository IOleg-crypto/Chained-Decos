#include "PhysicsComponent.h"
#include <raylib.h>
#include <raymath.h>



PhysicsComponent::PhysicsComponent() = default;

void PhysicsComponent::Update(float deltaTime) {
    m_dt = deltaTime;
    if (!m_isKinematic) {
        ApplyPhysics(deltaTime);
    }
}

void PhysicsComponent::ApplyPhysics(float deltaTime) {
    ApplyGravity(deltaTime);
    ApplyDrag(deltaTime);
    IntegrateForces(deltaTime);
}

void PhysicsComponent::ApplyGravity(float deltaTime) {
    if (!m_isGrounded) {
        m_forces.y -= m_gravity * deltaTime;
    }
}

void PhysicsComponent::ApplyDrag(float deltaTime) {
    m_forces = Vector3Add(m_forces, Vector3Scale(m_velocity, -m_drag * deltaTime));
}

void PhysicsComponent::IntegrateForces(float deltaTime) {
    // Apply forces to velocity
    Vector3 acceleration = Vector3Scale(m_forces, deltaTime);
    m_velocity = Vector3Add(m_velocity, acceleration);
    
    // Reset forces for next frame
    m_forces = {0.0f, 0.0f, 0.0f};
}

void PhysicsComponent::TryJump() {
    if (m_isGrounded) {
        m_velocity.y = m_jumpStrength;
        m_isGrounded = false;
        m_isJumping = true;
    }
}

void PhysicsComponent::Land() {
    m_velocity.y = 0.0f;
    m_isGrounded = true;
    m_isJumping = false;
}

void PhysicsComponent::HandleSurfaceInteraction(const SurfaceComponent* surface) {
    if (surface) {
        // Adjust physics properties based on surface type
        switch (surface->GetSurfaceType()) {
            case SurfaceType::Ice:
                m_drag = 0.01f;
                break;
            case SurfaceType::Mud:
                m_drag = 0.5f;
                break;
            case SurfaceType::Default:
            default:
                m_drag = 0.1f;
                break;
        }
    }
}

bool PhysicsComponent::HasExtremeVelocity() const {
    return Vector3Length(m_velocity) > MAX_SPEED;
}

bool PhysicsComponent::IsGrounded() const { return m_isGrounded; }
void PhysicsComponent::SetGroundLevel(bool isGrounded) { m_isGrounded = isGrounded; }
bool PhysicsComponent::IsKinematic() const { return m_isKinematic; }
void PhysicsComponent::SetKinematic(bool kinematic) { m_isKinematic = kinematic; }
bool PhysicsComponent::IsJumping() const { return m_isJumping; }
void PhysicsComponent::SetJumpState(bool jumping) { m_isJumping = jumping; }
Vector3 PhysicsComponent::GetVelocity() const { return m_velocity; }
void PhysicsComponent::SetVelocity(const Vector3 &velocity) { m_velocity = velocity; }
void PhysicsComponent::AddVelocity(const Vector3 &delta)
{
    m_velocity = Vector3Add(m_velocity, delta);
}
float PhysicsComponent::GetVelocityY() const { return m_velocity.y; }
void PhysicsComponent::SetVelocityY(float y) { m_velocity.y = y; }
void PhysicsComponent::CancelVerticalVelocity() { m_velocity.y = 0.0f; }
float PhysicsComponent::GetGravity() const { return m_gravity; }
void PhysicsComponent::SetGravity(float gravity) { m_gravity = gravity; }
float PhysicsComponent::GetJumpStrength() const { return m_jumpStrength; }
void PhysicsComponent::SetJumpStrength(float strength) { m_jumpStrength = strength; }
float PhysicsComponent::GetDrag() const { return m_drag; }
void PhysicsComponent::SetDrag(float drag) { m_drag = drag; }
float PhysicsComponent::GetDeltaTime() const { return m_dt; }
void PhysicsComponent::SetInAir() { m_isGrounded = false; }
