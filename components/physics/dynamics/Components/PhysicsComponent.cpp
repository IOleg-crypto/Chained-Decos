#include "PhysicsComponent.h"
#include <algorithm>
#include <cmath>
#include <future>
#include <raylib.h>
#include <raymath.h>
#include <thread>

LegacyPhysicsComponent::LegacyPhysicsComponent() = default;

void LegacyPhysicsComponent::Update(float deltaTime)
{
    m_deltaTime = deltaTime;
    if (!m_isKinematic)
    {
        ApplyPhysics(deltaTime);
    }
}

void LegacyPhysicsComponent::ApplyPhysics(float deltaTime)
{
    ApplyGravity(deltaTime);
    ApplyDrag(deltaTime);
    IntegrateAccumulatedForces(deltaTime);
}

void LegacyPhysicsComponent::ApplyGravity(float deltaTime)
{
    if (!m_isGrounded)
    {
        m_accumulatedForces.y -= m_gravity * deltaTime;
    }
}

void LegacyPhysicsComponent::ApplyDrag(float deltaTime)
{
    // Don't apply drag when grounded (you're not moving through air)
    if (!m_isGrounded)
    {
        m_accumulatedForces =
            Vector3Add(m_accumulatedForces, Vector3Scale(m_velocity, -m_drag * deltaTime));
    }
}

void LegacyPhysicsComponent::IntegrateAccumulatedForces(float deltaTime)
{
    // Apply forces to velocity
    Vector3 acceleration = Vector3Scale(m_accumulatedForces, deltaTime);
    m_velocity = Vector3Add(m_velocity, acceleration);

    // Reset forces for next frame
    m_accumulatedForces = {0.0f, 0.0f, 0.0f};
}

void LegacyPhysicsComponent::TryJump()
{
    if (m_isGrounded)
    {
        m_velocity.y = m_jumpStrength;
        m_isGrounded = false;
        m_isJumping = true;
    }
}

void LegacyPhysicsComponent::Land()
{
    m_velocity.y = 0.0f;
    m_isGrounded = true;
    m_isJumping = false;
}

void LegacyPhysicsComponent::HandleSurfaceInteraction(const SurfaceComponent *surface)
{
    if (surface)
    {
        // Adjust physics properties based on surface type
        switch (surface->GetSurfaceType())
        {
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

bool LegacyPhysicsComponent::HasExtremeVelocity() const
{
    return Vector3Length(m_velocity) > MAX_SPEED;
}

bool LegacyPhysicsComponent::IsGrounded() const
{
    return m_isGrounded;
}
void LegacyPhysicsComponent::SetGroundLevel(bool isGrounded)
{
    m_isGrounded = isGrounded;
}
bool LegacyPhysicsComponent::IsKinematic() const
{
    return m_isKinematic;
}
void LegacyPhysicsComponent::SetKinematic(bool kinematic)
{
    m_isKinematic = kinematic;
}
bool LegacyPhysicsComponent::IsJumping() const
{
    return m_isJumping;
}
void LegacyPhysicsComponent::SetJumpState(bool jumping)
{
    m_isJumping = jumping;
}
Vector3 LegacyPhysicsComponent::GetVelocity() const
{
    return m_velocity;
}
void LegacyPhysicsComponent::SetVelocity(const Vector3 &velocity)
{
    m_velocity = velocity;
}
void LegacyPhysicsComponent::AddVelocity(const Vector3 &delta)
{
    m_velocity = Vector3Add(m_velocity, delta);
}
float LegacyPhysicsComponent::GetVelocityY() const
{
    return m_velocity.y;
}
void LegacyPhysicsComponent::SetVelocityY(float y)
{
    m_velocity.y = y;
}
void LegacyPhysicsComponent::CancelVerticalVelocity()
{
    m_velocity.y = 0.0f;
}
float LegacyPhysicsComponent::GetGravity() const
{
    return m_gravity;
}
void LegacyPhysicsComponent::SetGravity(float gravity)
{
    m_gravity = gravity;
}
float LegacyPhysicsComponent::GetJumpStrength() const
{
    return m_jumpStrength;
}
void LegacyPhysicsComponent::SetJumpStrength(float strength)
{
    m_jumpStrength = strength;
}
float LegacyPhysicsComponent::GetDrag() const
{
    return m_drag;
}
void LegacyPhysicsComponent::SetDrag(float drag)
{
    m_drag = drag;
}
float LegacyPhysicsComponent::GetDeltaTime() const
{
    return m_deltaTime;
}
void LegacyPhysicsComponent::SetInAir()
{
    m_isGrounded = false;
}

// Static method for parallel physics updates
void LegacyPhysicsComponent::UpdatePhysicsComponentsParallel(
    std::vector<LegacyPhysicsComponent *> &physicsComponents, float deltaTime)
{
    if (physicsComponents.empty())
    {
        return;
    }

    const size_t numComponents = physicsComponents.size();
    const size_t parallelThreshold = 8; // Only use parallel processing if we have enough components

    if (numComponents < parallelThreshold)
    {
        // Fall back to sequential for small numbers of components
        for (LegacyPhysicsComponent *component : physicsComponents)
        {
            if (component)
            {
                component->Update(deltaTime);
            }
        }
        return;
    }

    // Split components into chunks for parallel processing
    size_t numThreads = std::min(static_cast<size_t>(std::thread::hardware_concurrency()),
                                 static_cast<size_t>(numComponents / 2));
    numThreads = std::max(numThreads, static_cast<size_t>(1));

    std::vector<std::future<void>> futures;
    size_t chunkSize = numComponents / numThreads;

    for (size_t i = 0; i < numThreads; ++i)
    {
        size_t startIdx = i * chunkSize;
        size_t endIdx = (i == numThreads - 1) ? numComponents : (i + 1) * chunkSize;

        futures.push_back(std::async(std::launch::async,
                                     [startIdx, endIdx, &physicsComponents, deltaTime]()
                                     {
                                         for (size_t j = startIdx; j < endIdx; ++j)
                                         {
                                             LegacyPhysicsComponent *component =
                                                 physicsComponents[j];
                                             if (component && !component->IsKinematic())
                                             {
                                                 component->Update(deltaTime);
                                             }
                                         }
                                     }));
    }

    // Wait for all threads to complete
    for (auto &future : futures)
    {
        future.wait();
    }
}
