#include "Player.h"
#include "PlayerMovement.h"

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
    m_player->SyncCollision();
}

Vector3 PlayerMovement::GetPosition() const { return m_position; }

float PlayerMovement::GetRotationY() const { return m_rotationY; }

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
        constexpr float MAX_FALL_SPEED = -50.0f;
        if (vel.y < MAX_FALL_SPEED)
        {
            vel.y = MAX_FALL_SPEED;
        }
        m_physics.SetVelocity(vel);
    }
    else
    {
        if (vel.y < 0.0f)
            vel.y = 0.0f;
        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    float deltaTime = GetFrameTime();
    Vector3 workingPos = GetPosition();

    // Small downward bias to keep contact with ground when descending
    if (velocity.y <= 0.0f)
        workingPos.y -= 0.005f;

    // 1) Vertical movement and resolution (with pre-ground raycast)
    Vector3 targetPos = workingPos;
    targetPos.y += velocity.y * deltaTime;
    SetPosition(targetPos);
    m_player->UpdatePlayerBox();
    m_player->UpdatePlayerCollision();

    bool groundedThisStep = false;
    Vector3 response;
    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        // Apply vertical correction only
        if (response.y > 0.0f)
        {
            // Push up from current target position to fully clear penetration
            targetPos.y += response.y + 0.001f;
            if (velocity.y <= 0.0f)
                groundedThisStep = true;
            velocity.y = 0.0f;
        }
        else if (response.y < 0.0f)
        {
            // Ceiling hit
            targetPos.y = workingPos.y + response.y;
            if (velocity.y > 0.0f)
                velocity.y = 0.0f;
        }
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();
        m_player->UpdatePlayerCollision();
    }
    else
    {
        // No penetration found: raycast down to catch precise mesh ground a tiny distance below
        float halfHeight = m_player->GetPlayerSize().y * 0.5f;
        Vector3 rayOrigin = {targetPos.x, targetPos.y + halfHeight, targetPos.z};
        float hitDist = 0.0f;
        Vector3 hitPt = {0};
        Vector3 hitN = {0};
        const float probe = halfHeight + 0.2f;
        if (collisionManager.RaycastDown(rayOrigin, probe, hitDist, hitPt, hitN))
        {
            float newBottom = hitPt.y + m_skinWidth;
            float currentBottom = targetPos.y - halfHeight;
            float deltaUp = newBottom - currentBottom;
            if (deltaUp > 0.0f && velocity.y <= 0.0f)
            {
                targetPos.y += deltaUp;
                SetPosition(targetPos);
                m_player->UpdatePlayerBox();
                m_player->UpdatePlayerCollision();
                groundedThisStep = true;
                velocity.y = 0.0f;
            }
        }
    }

    // 2) Horizontal movement and resolution
    workingPos = GetPosition();
    targetPos = workingPos;
    targetPos.x += velocity.x * deltaTime;
    targetPos.z += velocity.z * deltaTime;
    SetPosition(targetPos);
    m_player->UpdatePlayerBox();
    m_player->UpdatePlayerCollision();

    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        // Ignore vertical component during horizontal resolution to avoid pop
        response.y = 0.0f;
        targetPos = Vector3Add(workingPos, response);
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();
        m_player->UpdatePlayerCollision();
    }

    // Grounded hysteresis: require a few consecutive grounded frames to set true,
    // and a few consecutive ungrounded frames to clear. This prevents rapid toggling.
    constexpr int GROUNDED_SET_FRAMES = 2;
    constexpr int GROUNDED_CLEAR_FRAMES = 3;
    constexpr int COYOTE_FRAMES = 4; // allow brief grace period after losing ground

    if (groundedThisStep)
    {
        m_framesSinceGround = 0;
        if (!m_physics.IsGrounded())
        {
            // Set grounded after minimal stability frames
            m_physics.SetGroundLevel(true);
        }
        m_coyoteFramesRemaining = COYOTE_FRAMES;
    }
    else
    {
        m_framesSinceGround++;
        if (m_coyoteFramesRemaining > 0)
            m_coyoteFramesRemaining--;
        bool allowCoyote = (m_coyoteFramesRemaining > 0) && (m_physics.GetVelocity().y <= 0.0f);

        if (m_framesSinceGround >= GROUNDED_CLEAR_FRAMES && !allowCoyote)
        {
            m_physics.SetGroundLevel(false);
        }
    }

    return GetPosition();
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &responseMtv)
{
    Vector3 velocity = m_physics.GetVelocity();

    if (responseMtv.y > 0.001f && velocity.y <= 0.0f)
    {
        velocity.y = 0.0f;
        m_physics.SetGroundLevel(true);
    }
    else if (responseMtv.y < -0.001f && velocity.y > 0.0f)
    {
        velocity.y = 0.0f;
    }
    else
    {
        Vector3 normal = responseMtv;
        float len = Vector3Length(normal);
        if (len > 1e-6f)
            normal = Vector3Scale(normal, 1.0f / len);
        else
            normal = {0, 0, 0};

        float vn = Vector3DotProduct(velocity, normal);
        velocity = Vector3Subtract(velocity, Vector3Scale(normal, vn));
    }

    m_physics.SetVelocity(velocity);

    // Do not clear grounded state on horizontal contacts to avoid jitter
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();

    constexpr float SNAP_DISTANCE = 0.5f;
    Vector3 checkPos = m_position;
    checkPos.y -= SNAP_DISTANCE;
    SetPosition(checkPos);
    m_player->UpdatePlayerBox();

    Vector3 response;
    bool collide = collisionManager.CheckCollision(m_player->GetCollision(), response);

    SetPosition(m_position);
    m_player->UpdatePlayerBox();

    if (collide && response.y > 0.0f && fabs(response.y) >= fabs(response.x) &&
        fabs(response.y) >= fabs(response.z))
    {
        Vector3 pos = m_position;
        pos.y += response.y;
        SetPosition(pos);
        m_player->UpdatePlayerBox();
        velocity.y = 0.0f;
        m_physics.SetVelocity(velocity);
        m_physics.SetGroundLevel(true);
    }
    else
    {
        m_physics.SetGroundLevel(false);
    }
}

bool PlayerMovement::ExtractFromCollider()
{
    if (!m_lastCollisionManager)
        return false;

    Vector3 velocity = m_physics.GetVelocity();
    if (!m_physics.IsGrounded() || fabsf(velocity.y) > 0.1f)
        return false;

    Vector3 currentPos = GetPosition();
    m_player->UpdatePlayerBox();

    Vector3 response;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return false;

    Vector3 safePos = {currentPos.x, currentPos.y + 0.5f, currentPos.z};
    SetPosition(safePos);
    m_player->UpdatePlayerBox();

    Vector3 testResponse;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), testResponse))
        return true;

    SetPosition(Player::DEFAULT_SPAWN_POSITION);
    m_physics.SetVelocity({0, 0, 0});
    m_physics.SetGroundLevel(false);
    return true;
}

void PlayerMovement::SetRotationY(float rotation) { m_rotationY = rotation; }
