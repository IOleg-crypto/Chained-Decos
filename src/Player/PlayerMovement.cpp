#include <Player/Player.h>
#include <Player/PlayerMovement.h>
#include <raylib.h>

PlayerMovement::PlayerMovement(Player *player)
    : m_player(player), m_position(Player::DEFAULT_SPAWN_POSITION)
{
    // Initialize physics state
    m_physics.SetGroundLevel(false);  // Start in air
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
    Vector3 vel = m_physics.GetVelocity();

    // If the player is not on the ground, apply gravity
    if (!m_physics.IsGrounded())
    {
        vel.y -= m_physics.GetGravity() * deltaTime;

        // Limit falling speed
        constexpr float MAX_FALL_SPEED = -50.0f;
        if (vel.y < MAX_FALL_SPEED)
            vel.y = MAX_FALL_SPEED;

        m_physics.SetVelocity(vel);
    }
    // If the player is on the ground, reset vertical velocity
    else
    {
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);

        // Important: do not reset jumpState until the player leaves the ground
        // m_player->GetPhysics().SetJumpState(false);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    float deltaTime = GetFrameTime();
    Vector3 workingPos = GetPosition();

    constexpr int subSteps = 4;
    Vector3 subStepVelocity = Vector3Scale(velocity, deltaTime / subSteps);

    m_physics.SetGroundLevel(false); // Reset grounded state at the start of the step

    for (int i = 0; i < subSteps; i++)
    {
        Vector3 targetPos = Vector3Add(workingPos, subStepVelocity);
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();

        Vector3 response;
        if (collisionManager.CheckCollision(m_player->GetCollision(), response))
        {
            HandleCollisionVelocity(response);

            // If this is ground (Y dominates), apply only vertical correction
            Vector3 mtvToApply = response;
            if (fabs(response.y) >= fabs(response.x) && fabs(response.y) >= fabs(response.z))
            {
                mtvToApply = {0.0f, response.y, 0.0f};
            }

            // Minimal push to exit collision
            targetPos = Vector3Add(workingPos, Vector3Scale(mtvToApply, 1.01f));
            SetPosition(targetPos);
            m_player->UpdatePlayerBox();
        }

        workingPos = GetPosition();
    }

    // After all substeps, check if there is ground under the player
    SnapToGround(collisionManager);

    return workingPos;
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &responseNormal)
{
    Vector3 velocity = m_physics.GetVelocity();

    // Ground collision
    if (responseNormal.y > 0.1f)
    {
        if (velocity.y <= 0.0f)
        {
            velocity.y = 0.0f;
            m_physics.SetGroundLevel(true);
        }
        return;
    }

    // Ceiling collision
    if (responseNormal.y < -0.1f)
    {
        if (velocity.y > 0.0f)
            velocity.y = 0.0f;
    }
    // Walls
    else
    {
        float vn = Vector3DotProduct(velocity, responseNormal);
        velocity = Vector3Subtract(velocity, Vector3Scale(responseNormal, vn));
    }

    m_physics.SetVelocity(velocity);
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    Vector3 position = GetPosition();

    // If the player is moving up, do not snap to ground
    if (velocity.y > 0.0f)
    {
        m_physics.SetGroundLevel(false);
        return;
    }

    const float SNAP_DISTANCE = 0.01f; // Can be adjusted to match player size

    // Check slightly below the current position
    Vector3 checkPos = position;
    checkPos.y -= SNAP_DISTANCE;

    SetPosition(checkPos);
    m_player->UpdatePlayerBox();

    Vector3 response;
    bool collide = collisionManager.CheckCollision(m_player->GetCollision(), response);

    // Restore the player's original position
    SetPosition(position);
    m_player->UpdatePlayerBox();

    if (collide && fabs(response.y) >= fabs(response.x) && fabs(response.y) >= fabs(response.z))
    {
        // There is ground under the player — snap only along Y
        position.y += response.y;
        SetPosition(position);
        m_player->UpdatePlayerBox();

        velocity.y = 0.0f;
        m_physics.SetVelocity(velocity);
        m_physics.SetGroundLevel(true);
    }
    else
    {
        // No ground — player is in the air
        m_physics.SetGroundLevel(false);
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

void PlayerMovement::SetRotationY(float rotation) { this->m_rotationY = rotation; }
