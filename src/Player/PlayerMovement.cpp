#include <Player/Player.h>
#include <Player/PlayerMovement.h>
#include <raylib.h>

PlayerMovement::PlayerMovement(Player *player)
    : m_player(player), m_position(Player::DEFAULT_SPAWN_POSITION)
{
    // Initialize physics state
    m_physics.SetGroundLevel(true);   // Start in air
    m_physics.SetVelocity({0, 0, 0}); // No initial velocity

    m_lastCollisionManager = nullptr;
}

void PlayerMovement::Move(const Vector3 &moveVector)
{
    m_position = Vector3Add(m_position, moveVector);
}

void PlayerMovement::SetPosition(const Vector3 &pos)
{
    m_position = pos;
    m_player->UpdatePlayerBox();
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
    {
        TraceLog(LOG_WARNING, "Jump ignored - not grounded (vel.y: %.2f)",
                 m_physics.GetVelocity().y);
        return;
    }

    TraceLog(LOG_INFO, "Jump impulse received: %.2f", impulse);

    float verticalVelocity = impulse;
    Vector3 currentVelocity = m_physics.GetVelocity();

    Vector3 jumpVelocity = {currentVelocity.x, verticalVelocity, currentVelocity.z};

    m_physics.SetVelocity(jumpVelocity);

    m_physics.SetGroundLevel(false);
    m_player->GetPhysics().SetJumpState(true);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();

        vel.y -= m_physics.GetGravity() * deltaTime;

        const float MAX_FALL_SPEED = -50.0f;
        if (vel.y < MAX_FALL_SPEED)
            vel.y = MAX_FALL_SPEED;

        m_physics.SetVelocity(vel);
        m_player->GetPhysics().SetJumpState(false);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    float deltaTime = GetFrameTime();
    Vector3 currentPos = GetPosition();

    // Apply movement in smaller steps to improve collision detection accuracy
    const int subSteps = 4;
    Vector3 subStepVelocity = Vector3Scale(velocity, deltaTime / subSteps);
    Vector3 workingPos = currentPos;

    for (int i = 0; i < subSteps; i++)
    {
        Vector3 targetPos = Vector3Add(workingPos, subStepVelocity);
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();

        Vector3 response;
        bool hasCollision = collisionManager.CheckCollision(m_player->GetCollision(), response);

        if (hasCollision)
        {
            // Move back to previous position
            SetPosition(workingPos);
            m_player->UpdatePlayerBox();

            // Apply collision response
            Vector3 correctedPos = Vector3Add(workingPos, response);
            SetPosition(correctedPos);
            m_player->UpdatePlayerBox();

            HandleCollisionVelocity(response);

            // Update working position for next sub-step
            workingPos = correctedPos;
        }
        else
        {
            workingPos = targetPos;
        }
    }

    return workingPos;
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &response)
{
    Vector3 velocity = m_physics.GetVelocity();

    float absX = fabsf(response.x);
    float absY = fabsf(response.y);
    float absZ = fabsf(response.z);

    if (absY > absX && absY > absZ)
    {
        if (response.y > 0.0f)
        {
            TraceLog(LOG_INFO, "Floor collision - landing");
            velocity.y = 0.0f;
            m_physics.SetGroundLevel(true);
        }
        else
        {
            TraceLog(LOG_INFO, "Ceiling collision");
            velocity.y = 0.0f;
        }
    }
    else
    {
        TraceLog(LOG_INFO, "Wall collision");

        if (absX > absZ)
        {
            velocity.x = 0.0f;
        }
        else
        {
            velocity.z = 0.0f;
        }
    }

    m_physics.SetVelocity(velocity);
}

void PlayerMovement::SnapToGroundIfNeeded(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    Vector3 position = GetPosition();

    // Only snap to ground when falling or just landed
    if (velocity.y > 0.0f)
    {
        return;
    }

    const float SNAP_DISTANCE = 0.5f; // Reduced snap distance for more precise snapping
    Vector3 checkPos = position;
    checkPos.y -= SNAP_DISTANCE;

    SetPosition(checkPos);
    m_player->UpdatePlayerBox();

    Vector3 response;
    bool foundGround = collisionManager.CheckCollision(m_player->GetCollision(), response);

    if (foundGround && response.y > 0.0f)
    {
        float correctY = checkPos.y + response.y + 0.01f;
        Vector3 groundPos = position;
        groundPos.y = correctY;
        SetPosition(groundPos);
        m_player->UpdatePlayerBox();

        velocity.y = 0.0f;
        m_physics.SetVelocity(velocity);
        m_physics.SetGroundLevel(true);

        TraceLog(LOG_INFO, "⬇️ Snapped to ground: %.3f -> %.3f", position.y, correctY);
    }
    else
    {
        SetPosition(position);
        m_player->UpdatePlayerBox();

        // Only set as not grounded if we're actually falling
        if (velocity.y < -0.1f)
        {
            m_physics.SetGroundLevel(false);
        }
    }
}

bool PlayerMovement::ExtractFromCollider()
{
    if (!m_lastCollisionManager)
    {
        TraceLog(LOG_ERROR, "🚨 Cannot extract player - no collision manager reference!");
        return false;
    }

    Vector3 velocity = m_physics.GetVelocity();
    if (!m_physics.IsGrounded() || fabsf(velocity.y) > 0.1f)
    {
        TraceLog(LOG_INFO, "🚨 Player in air - skipping extraction (vel.y: %.2f)", velocity.y);
        return false;
    }

    Vector3 currentPos = GetPosition();
    m_player->UpdatePlayerBox();

    Vector3 response = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
    {
        return false;
    }

    TraceLog(LOG_WARNING, "🚨 EXTRACTING PLAYER FROM COLLIDER - current pos: (%.2f, %.2f, %.2f)",
             currentPos.x, currentPos.y, currentPos.z);

    Vector3 safePos = {currentPos.x, currentPos.y + 0.5f, currentPos.z};
    SetPosition(safePos);
    m_player->UpdatePlayerBox();

    Vector3 testResponse = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), testResponse))
    {
        TraceLog(LOG_INFO, "trEmergency extraction successful at: (%.2f, %.2f, %.2f)", safePos.x,
                 safePos.y, safePos.z);
        return true;
    }

    SetPosition(Player::DEFAULT_SPAWN_POSITION);
    m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
    m_physics.SetGroundLevel(false);
    TraceLog(LOG_ERROR, "CRITICAL: Emergency teleport to spawn");
    return true;
}