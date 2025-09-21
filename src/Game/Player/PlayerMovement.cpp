#include "PlayerMovement.h"
#include "Player.h"
#include <cmath>
#include <raylib.h>

PlayerMovement::PlayerMovement(Player *player)
    : m_player(player), m_position(Player::DEFAULT_SPAWN_POSITION), m_rotationY(0.0f),
      m_walkSpeed(11.0f)
{
    m_physics.SetGroundLevel(false);
    m_physics.SetVelocity({0, 0, 0});
}

void PlayerMovement::Move(const Vector3 &moveVector)
{
    m_position = Vector3Add(m_position, moveVector);
}

void PlayerMovement::SetPosition(const Vector3 &pos)
{
    m_position = pos;
    // Sync collision only when position actually changes
    m_player->SyncCollision();
}

Vector3 PlayerMovement::GetPosition() const { return m_position; }
float PlayerMovement::GetRotationY() const { return m_rotationY; }
void PlayerMovement::SetRotationY(float rotation) { m_rotationY = rotation; }

float PlayerMovement::GetSpeed() { return m_walkSpeed; }
void PlayerMovement::SetSpeed(float speed) { m_walkSpeed = speed; }

PhysicsComponent &PlayerMovement::GetPhysics() { return m_physics; }
const PhysicsComponent &PlayerMovement::GetPhysics() const { return m_physics; }

void PlayerMovement::SetCollisionManager(const CollisionManager *collisionManager)
{
    m_lastCollisionManager = collisionManager;
}

void PlayerMovement::ApplyJumpImpulse(float impulse)
{
    if (!m_physics.IsGrounded())
        return;
    Vector3 vel = m_physics.GetVelocity();
    vel.y = impulse;
    m_physics.SetVelocity(vel);
    m_physics.SetGroundLevel(false);
    m_player->GetPhysics().SetJumpState(true);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    Vector3 vel = m_physics.GetVelocity();
    if (!m_physics.IsGrounded())
    {
        vel.y -= m_physics.GetGravity() * deltaTime;
        // Clamp fall speed softly
        const float MAX_FALL_SPEED = -25.0f;
        vel.y = std::max(vel.y, MAX_FALL_SPEED);
        m_physics.SetVelocity(vel);
    }
    else if (vel.y < 0.0f)
    {
        // Smoothly dampen residual negative velocity on landing
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    float dt = GetFrameTime();
    Vector3 vel = m_physics.GetVelocity();
    Vector3 targetPos = m_position;

    // 1) Vertical movement
    targetPos.y += vel.y * dt;
    SetPosition(targetPos);

    Vector3 response = {0};
    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        // Validate collision response to prevent invalid movements
        response = ValidateCollisionResponse(response, targetPos);

        if (fabsf(response.y) > 0.0001f)
        {
            targetPos.y += response.y;
            SetPosition(targetPos);
            // Landing or ceiling hit
            if (vel.y <= 0.0f && response.y > 0.0f)
            {
                vel.y = 0.0f;
                m_physics.SetGroundLevel(true);
                // Ensure player is properly positioned on ground
                m_position.y = targetPos.y;
            }
            else if (vel.y > 0.0f && response.y < 0.0f)
            {
                vel.y = 0.0f;
            }
        }
    }

    // 2) Horizontal movement
    targetPos.x += vel.x * dt;
    targetPos.z += vel.z * dt;
    SetPosition(targetPos);

    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        response.y = 0.0f;
        // Validate horizontal collision response
        response = ValidateCollisionResponse(response, targetPos);
        targetPos = Vector3Add(m_position, response);
        SetPosition(targetPos);
    }

    m_physics.SetVelocity(vel);

    // 3) Update grounded state
    UpdateGrounded(collisionManager);
    
    return m_position;
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &responseMtv)
{
    Vector3 vel = m_physics.GetVelocity();

    if (responseMtv.y > 0.001f && vel.y <= 0.0f)
    {
        vel.y = 0.0f;
        m_physics.SetGroundLevel(true);
    }
    else
    {
        Vector3 normal = responseMtv;
        float len = Vector3Length(normal);
        if (len > 1e-6f)
            normal = Vector3Scale(normal, 1.0f / len);
        float vn = Vector3DotProduct(vel, normal);
        vel = Vector3Subtract(vel, Vector3Scale(normal, vn));
    }

    m_physics.SetVelocity(vel);
}

void PlayerMovement::UpdateGrounded(const CollisionManager &collisionManager)
{
    const Vector3 size = m_player->GetPlayerSize();
    const Vector3 center = m_position;
    // Increase raycast distance to ensure we can reach the ground
    const float maxDistance = size.y + 15.0f; // Increased from 1.0f to 15.0f
    float hitDist = 0.0f;
    Vector3 hitPoint = {0};
    Vector3 hitNormal = {0};
    bool grounded = false;

    if (collisionManager.RaycastDown(center, maxDistance, hitDist, hitPoint, hitNormal))
    {
        float bottom = center.y - size.y * 0.5f;
        float gap = hitPoint.y - bottom;
        // Only consider grounded when moving down/standing to avoid breaking jumps
        if (m_physics.GetVelocity().y <= 0.0f)
        {
            // Increase tolerance for ground detection and add minimum gap check
            grounded = (gap >= -0.1f && gap <= 0.8f); // Allow slight penetration and larger tolerance
        }
    }

    // Additional check: if we're very close to ground level and not moving up fast, consider grounded
    if (!grounded && m_physics.GetVelocity().y <= 1.0f)
    {
        float bottom = center.y - size.y * 0.5f;
        if (bottom <= 0.5f) // Close to ground level
        {
            grounded = true;
        }
    }

    m_physics.SetGroundLevel(grounded);
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    // Raycast down without changing position
    const Vector3 size = m_player->GetPlayerSize();
    const Vector3 center = m_position;
    // Increase raycast distance to ensure we can reach the ground
    const float maxDistance = size.y + 15.0f; // Increased from 1.0f to 15.0f
    float hitDist = 0.0f;
    Vector3 hitPoint = {0};
    Vector3 hitNormal = {0};

    if (collisionManager.RaycastDown(center, maxDistance, hitDist, hitPoint, hitNormal))
    {
        // If close to ground — snap to it
        const float snapThreshold = 0.8f; // Increased from 0.6f to 0.8f
        float bottom = center.y - size.y * 0.5f;
        float gap = hitPoint.y - bottom;
        if (gap >= 0.0f && gap <= snapThreshold)
        {
            Vector3 newPos = m_position;
            newPos.y = hitPoint.y + size.y * 0.5f;
            SetPosition(newPos);
            Vector3 vel = m_physics.GetVelocity();
            vel.y = 0.0f;
            m_physics.SetVelocity(vel);
            m_physics.SetGroundLevel(true);
            return;
        }
    }

    m_physics.SetGroundLevel(false);
}

bool PlayerMovement::ExtractFromCollider()
{
    if (!m_lastCollisionManager)
        return false;

    if (!m_physics.IsGrounded() || fabsf(m_physics.GetVelocity().y) > 0.1f)
        return false;

    Vector3 response;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return false;

    SetPosition(Vector3Add(m_position, {0, 0.5f, 0}));

    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return true;

    SetPosition(Player::DEFAULT_SPAWN_POSITION);
    m_physics.SetVelocity({0, 0, 0});
    m_physics.SetGroundLevel(false);
    return true;
}

// New function to validate collision response and prevent invalid movements
Vector3 PlayerMovement::ValidateCollisionResponse(const Vector3& response, const Vector3& currentPosition)
{
    Vector3 validatedResponse = response;

    // Don't allow responses that would push player below ground level
    if (response.y < 0.0f && (currentPosition.y + response.y) < -5.0f)
    {
        validatedResponse.y = 0.0f;
        TraceLog(LOG_WARNING, "PlayerMovement::ValidateCollisionResponse() - Prevented player from going below ground");
    }

    // Don't allow large upward responses that could cause teleportation
    if (response.y > 0.0f && response.y > 2.0f)
    {
        validatedResponse.y = 2.0f;
        TraceLog(LOG_WARNING, "PlayerMovement::ValidateCollisionResponse() - Clamped excessive upward response");
    }

    // Don't allow large horizontal responses that could cause teleportation
    float horizontalMagnitude = sqrtf(response.x * response.x + response.z * response.z);
    if (horizontalMagnitude > 1.5f)
    {
        float scale = 1.5f / horizontalMagnitude;
        validatedResponse.x *= scale;
        validatedResponse.z *= scale;
        TraceLog(LOG_WARNING, "PlayerMovement::ValidateCollisionResponse() - Clamped excessive horizontal response");
    }

    return validatedResponse;
}

