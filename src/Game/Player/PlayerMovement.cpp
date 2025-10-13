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
    // More forgiving ground check for jumping
    if (!m_physics.IsGrounded() && m_physics.GetVelocity().y > 5.0f)
        return;

    Vector3 vel = m_physics.GetVelocity();
    vel.y = impulse;
    m_physics.SetVelocity(vel);
    m_physics.SetGroundLevel(false);
    m_player->GetPhysics().SetJumpState(true);

    TraceLog(LOG_DEBUG, "PlayerMovement::ApplyJumpImpulse() - Applied jump impulse: %.2f, velocity y: %.2f", impulse, vel.y);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    Vector3 vel = m_physics.GetVelocity();
    if (!m_physics.IsGrounded())
    {
        vel.y -= m_physics.GetGravity() * deltaTime;
        // Clamp fall speed softly
        const float MAX_FALL_SPEED = -60.0f;
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
    // Use default delta time if no window is available (for testing)
    float dt = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);
    Vector3 vel = m_physics.GetVelocity();
    Vector3 targetPos = m_position;

    // 1) Vertical movement
    targetPos.y += vel.y * dt;
    SetPosition(targetPos);

    Vector3 response = {0};
    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        // De-jitter: damp tiny horizontal corrections while walking
        if (fabsf(response.y) < 1e-4f)
        {
            float horiz = sqrtf(response.x*response.x + response.z*response.z);
            if (horiz > 0.0f && horiz <= 0.15f)
            {
                response.x = 0.0f;
                response.z = 0.0f;
            }
        }
        // Step-up heuristic: if horizontal collision and small step ahead while moving forward
        if (fabsf(response.y) < 1e-4f)
        {
            const float stepMax = 0.85f; // max step height
            Vector3 forward = { vel.x, 0.0f, vel.z };
            float speed = sqrtf(forward.x*forward.x + forward.z*forward.z);
            if (speed > 0.01f)
            {
                // probe slightly ahead and up
                Vector3 probePos = targetPos;
                probePos.y += stepMax;
                SetPosition(probePos);
                Vector3 probeResp = {0};
                if (!collisionManager.CheckCollision(m_player->GetCollision(), probeResp))
                {
                    // allow stepping up: cancel horizontal pushback
                    response.x = 0.0f;
                    response.z = 0.0f;
                }
                SetPosition(targetPos); // restore before applying validated response
            }
        }

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
        // Shape horizontal MTV to be stable: correct only opposite to movement direction
        {
            Vector3 horiz = {response.x, 0.0f, response.z};
            float horizLen = sqrtf(horiz.x*horiz.x + horiz.z*horiz.z);
            Vector3 move = {vel.x, 0.0f, vel.z};
            float speed = sqrtf(move.x*move.x + move.z*move.z);
            if (horizLen > 0.0f && speed > 0.01f)
            {
                // Desired correction along -move direction
                move.x /= speed; move.z /= speed; // normalize
                float proj = -(horiz.x*move.x + horiz.z*move.z); // magnitude opposite movement
                if (proj < 0.0f) proj = -proj; // ensure positive magnitude
                response.x = -move.x * proj;
                response.z = -move.z * proj;
            }
            else if (horizLen <= 0.15f)
            {
                // Ignore tiny lateral nudges to avoid left-right jitter
                response.x = 0.0f;
                response.z = 0.0f;
            }
        }
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
    // Set reasonable raycast distance - should be enough to reach ground but not excessive
    const float maxDistance = size.y + 2.0f;
    float hitDist = 0.0f;
    Vector3 hitPoint = {0};
    Vector3 hitNormal = {0};
    bool grounded = false;

    // Multi-sample footprint to bridge tiny holes
    const Vector3 offsets[] = {
        {0.0f, 0.0f, 0.0f},
        { size.x * 0.25f, 0.0f, 0.0f},
        {-size.x * 0.25f, 0.0f, 0.0f},
        {0.0f, 0.0f,  size.z * 0.25f},
        {0.0f, 0.0f, -size.z * 0.25f},
        { size.x * 0.25f, 0.0f,  size.z * 0.25f},
        {-size.x * 0.25f, 0.0f,  size.z * 0.25f},
        { size.x * 0.25f, 0.0f, -size.z * 0.25f},
        {-size.x * 0.25f, 0.0f, -size.z * 0.25f}
    };

    float bestGap = 1e9f;
    Vector3 bestPoint = {0};
    Vector3 bestNormal = {0};

    for (const Vector3 &off : offsets)
    {
        Vector3 probe = { center.x + off.x, center.y, center.z + off.z };
        float d = 0.0f; Vector3 p = {0}; Vector3 n = {0};
        if (collisionManager.RaycastDown(probe, maxDistance, d, p, n))
        {
            float bottom = center.y - size.y * 0.5f;
            float gap = p.y - bottom;
            if (gap < bestGap) { bestGap = gap; bestPoint = p; bestNormal = n; }

            if (m_physics.GetVelocity().y <= 0.0f)
            {
                const float maxSlopeDeg = 65.0f; // Increased for better slope handling
                const float maxSlopeCos = cosf(maxSlopeDeg * DEG2RAD);
                float upDot = Vector3DotProduct(n, {0.0f, 1.0f, 0.0f});
                bool withinGap = (gap >= -0.3f && gap <= 1.2f); // More generous gap range for jumping
                bool withinSlope = (upDot >= maxSlopeCos);
                if (withinGap && withinSlope)
                {
                    grounded = true;
                    hitPoint = p;
                    hitNormal = n;
                    break;
                }
            }
        }
    }

    if (!grounded && bestGap < 0.5f && m_physics.GetVelocity().y <= 0.0f)
    {
        grounded = true;
        hitPoint = bestPoint;
        hitNormal = bestNormal;
    }

    // Additional check: if we're very close to ground level and not moving up fast, consider grounded
    if (!grounded && m_physics.GetVelocity().y <= 2.0f)
    {
        float bottom = center.y - size.y * 0.5f;
        // Account for MODEL_Y_OFFSET when checking ground proximity
        // The visual model is offset by -1.0f, so we need to check against a higher threshold
        // The effective ground level for the collision box should be MODEL_Y_OFFSET higher than visual ground
        if (bottom <= (1.5f + fabsf(Player::MODEL_Y_OFFSET)))
        {
            grounded = true;
            TraceLog(LOG_DEBUG, "PlayerMovement::UpdateGrounded() - Force grounded due to proximity: bottom=%.2f", bottom);
        }
    }

    m_physics.SetGroundLevel(grounded);

    // Last safe position to avoid falling through tiny gaps
    static Vector3 s_lastSafePos = {0};
    static bool s_hasSafe = false;
    if (grounded)
    {
        // Account for MODEL_Y_OFFSET when calculating safe position
        s_lastSafePos = { center.x, hitPoint.y + size.y * 0.5f + Player::MODEL_Y_OFFSET, center.z };
        s_hasSafe = true;
    }
    else if (s_hasSafe && m_physics.GetVelocity().y <= 0.0f)
    {
        float drop = s_lastSafePos.y - center.y;
        if (drop >= -0.2f && drop <= 0.4f)
        {
            Vector3 newPos = m_position;
            newPos.y = s_lastSafePos.y;
            SetPosition(newPos);
        }
    }
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    // Raycast down without changing position
    const Vector3 size = m_player->GetPlayerSize();
    const Vector3 center = m_position;
    // Set reasonable raycast distance - should be enough to reach ground but not excessive
    const float maxDistance = size.y + 2.0f;
    float hitDist = 0.0f;
    Vector3 hitPoint = {0};
    Vector3 hitNormal = {0};

    if (collisionManager.RaycastDown(center, maxDistance, hitDist, hitPoint, hitNormal))
    {
        // If close to ground — snap to it
        const float snapThreshold = 1.2f; // Increased for better snapping
        float bottom = center.y - size.y * 0.5f;
        float gap = hitPoint.y - bottom;
        if (gap >= 0.0f && gap <= snapThreshold)
        {
            Vector3 newPos = m_position;
            // Account for MODEL_Y_OFFSET when positioning collision box
            // The collision box should be positioned so the visual model (offset by -1.0f) sits on the ground
            newPos.y = hitPoint.y + size.y * 0.5f - Player::MODEL_Y_OFFSET;
            SetPosition(newPos);
            Vector3 vel = m_physics.GetVelocity();
            vel.y = 0.0f;
            m_physics.SetVelocity(vel);
            m_physics.SetGroundLevel(true);
            TraceLog(LOG_DEBUG, "PlayerMovement::SnapToGround() - Snapped to ground: collision_box_y=%.2f, visual_model_y=%.2f",
                     newPos.y, newPos.y + Player::MODEL_Y_OFFSET);
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

    // Ignore small horizontal pushbacks while airborne and moving upward (BVH false positives in narrow gaps)
    if (!m_physics.IsGrounded() && m_physics.GetVelocity().y > 0.0f)
    {
        float horizMag = sqrtf(response.x * response.x + response.z * response.z);
        if (fabsf(response.y) < 1e-4f && horizMag > 0.0f && horizMag <= 0.35f)
        {
            validatedResponse.x = 0.0f;
            validatedResponse.z = 0.0f;
        }
    }

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

