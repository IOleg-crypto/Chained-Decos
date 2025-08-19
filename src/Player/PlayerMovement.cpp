#include <Player/PlayerMovement.h>
#include <Player/Player.h>

PlayerMovement::PlayerMovement(Player* player) 
    : m_player(player), m_position(Player::DEFAULT_SPAWN_POSITION)
{
    // Initialize physics state
    m_physics.SetGroundLevel(false); // Start in air
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

Vector3 PlayerMovement::GetPosition() const
{
    return m_position;
}

float PlayerMovement::GetRotationY() const
{
    return m_rotationY;
}

float PlayerMovement::GetSpeed()
{
    return m_walkSpeed;
}

void PlayerMovement::SetSpeed(float speed)
{
    m_walkSpeed = speed;
}

PhysicsComponent &PlayerMovement::GetPhysics()
{
    return m_physics;
}

const PhysicsComponent &PlayerMovement::GetPhysics() const
{
    return m_physics;
}

void PlayerMovement::SetCollisionManager(const CollisionManager* collisionManager)
{
    m_lastCollisionManager = collisionManager;
}

void PlayerMovement::ApplyJumpImpulse(float impulse)
{
    if (!m_physics.IsGrounded())
        return;

    // Calculate jump velocity
    float mass = 1.0f; // Use player's mass calculation from original code
    float verticalVelocity = impulse / mass;

    // Get current horizontal velocity to preserve momentum
    Vector3 currentVel = m_physics.GetVelocity();
    
    // Set jump velocity (preserve horizontal movement, add vertical impulse)
    Vector3 jumpVelocity = {currentVel.x, verticalVelocity, currentVel.z};
    
    m_physics.SetVelocity(jumpVelocity);
    m_physics.SetGroundLevel(false);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * deltaTime;

        const float MAX_FALL_SPEED = -20.0f;
        if (vel.y < MAX_FALL_SPEED) vel.y = MAX_FALL_SPEED;

        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 move = Vector3Scale(m_physics.GetVelocity(), GetFrameTime());
    Vector3 newPosition = GetPosition();

    float moveDistance = Vector3Length(move);
    if (moveDistance < 0.001f) return newPosition;

    int steps = (int)(moveDistance / 0.005f) + 1;
    steps = steps > 200 ? 200 : steps;
    Vector3 stepMove = Vector3Scale(move, 1.0f / steps);

    for (int i = 0; i < steps; i++)
    {
        Vector3 testPos = Vector3Add(newPosition, stepMove);
        SetPosition(testPos);
        m_player->UpdatePlayerBox();

        Vector3 response;
        if (collisionManager.CheckCollision(m_player->GetCollision(), response))
        {
            ResolveCollision(response);
            newPosition = GetPosition();
            break;
        }
        else
        {
            newPosition = testPos;
        }
    }

    return newPosition;
}

void PlayerMovement::ResolveCollision(const Vector3 &response)
{
    Vector3 velocity = m_physics.GetVelocity();
    float absX = fabsf(response.x);
    float absY = fabsf(response.y);
    float absZ = fabsf(response.z);

    // Floor/ceiling collision
    if (absY >= absX && absY >= absZ)
    {
        if (response.y > 0.0f) // Floor
        {
            Vector3 pos = GetPosition();
            pos.y += response.y + 0.05f;
            SetPosition(pos);
            m_physics.SetGroundLevel(true);
            velocity.y = 0.0f;
        }
        else // Ceiling
        {
            Vector3 pos = GetPosition();
            pos.y += response.y;
            SetPosition(pos);
            velocity.y = 0.0f;
        }
    }
    else // Wall collision
    {
        Vector3 pos = GetPosition();
        if (absX > absZ)
        {
            pos.x += response.x;
            velocity.x = 0.0f;
        }
        else
        {
            pos.z += response.z;
            velocity.z = 0.0f;
        }
        SetPosition(pos);
    }

    m_physics.SetVelocity(velocity);
}

void PlayerMovement::SnapToGroundIfNeeded(const CollisionManager &collisionManager)
{
    Vector3 pos = GetPosition();
    float groundTop = PhysicsComponent::GROUND_COLLISION_CENTER.y +
                      PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;

    // Simple snap to world floor
    if (!m_physics.IsGrounded() && pos.y <= groundTop + 0.1f)
    {
        pos.y = groundTop + 0.05f;
        SetPosition(pos);
        Vector3 vel = m_physics.GetVelocity();
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);
        m_physics.SetGroundLevel(true);
    }

    // Optional: extra thin-floor detection via raycast around player
    if (!m_physics.IsGrounded())
    {
        const float RAY_DIST = 0.2f;
        Vector3 offsets[] = {
            {0,0,0}, {0.3f,0,0}, {-0.3f,0,0}, {0,0,0.3f}, {0,0,-0.3f},
            {0.2f,0,0.2f}, {-0.2f,0,0.2f}, {0.2f,0,-0.2f}, {-0.2f,0,-0.2f}
        };

        for (Vector3 off : offsets)
        {
            Vector3 testPos = pos;
            testPos.x += off.x;
            testPos.z += off.z;
            testPos.y -= RAY_DIST;

            SetPosition(testPos);
            m_player->UpdatePlayerBox();

            Vector3 floorResponse;
            if (collisionManager.CheckCollision(m_player->GetCollision(), floorResponse) && floorResponse.y > 0.01f)
            {
                float maxH = fmaxf(fabsf(floorResponse.x), fabsf(floorResponse.z));
                if (floorResponse.y > maxH * 0.8f)
                {
                    pos.y = testPos.y + floorResponse.y + 0.15f;
                    SetPosition(pos);
                    Vector3 vel = m_physics.GetVelocity();
                    vel.y = 0.0f;
                    m_physics.SetVelocity(vel);
                    m_physics.SetGroundLevel(true);
                    break;
                }
            }
        }
    }
}

Vector3 PlayerMovement::ClampMovementPerFrame(const Vector3 &movement, float maxMove)
{
    float len = Vector3Length(movement);
    if (len > maxMove) {
        TraceLog(LOG_INFO, "🚀 Movement clamped from %.3f to %.3f units", len, maxMove);
        return Vector3Scale(Vector3Normalize(movement), maxMove);
    }
    return movement;
}

bool PlayerMovement::TryStepUp(const Vector3 &targetPos, const Vector3 &response)
{
    const float MAX_STEP_HEIGHT = 0.3f;
    if (!m_physics.IsGrounded() || response.y <= 0.01f || response.y >= MAX_STEP_HEIGHT)
        return false;

    Vector3 stepUpPos = targetPos;
    stepUpPos.y += response.y + 0.05f;
    SetPosition(stepUpPos);
    m_player->UpdatePlayerBox();

    Vector3 stepResp = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), stepResp)) {
        TraceLog(LOG_INFO, "🪜 Step up successful, height: %.2f", response.y);
        return true;
    }

    SetPosition(targetPos); // відкат якщо не вдалося
    m_player->UpdatePlayerBox();
    return false;
}

void PlayerMovement::WallSlide(const Vector3 &currentPos, const Vector3 &movement, const Vector3 &response)
{
    Vector3 wallNormal = Vector3Normalize(response);
    Vector3 slideDir = Vector3Subtract(movement, Vector3Scale(wallNormal, Vector3DotProduct(movement, wallNormal)));
    slideDir = Vector3Scale(slideDir, 0.8f);

    Vector3 slidePos = Vector3Add(currentPos, slideDir);
    SetPosition(slidePos);
    m_player->UpdatePlayerBox();

    Vector3 slideResp = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), slideResp)) {
        TraceLog(LOG_INFO, "🚧 Wall sliding successful");
    } else {
        slidePos = Vector3Add(currentPos, Vector3Scale(response, 1.1f));
        SetPosition(slidePos);
        m_player->UpdatePlayerBox();
        TraceLog(LOG_INFO, "🚧 Wall sliding failed, applied collision response");
    }
}

void PlayerMovement::ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    if (Vector3Length(worldMoveDir) < 0.001f) return;

    // Поворот гравця
    m_rotationY = atan2f(worldMoveDir.x, worldMoveDir.z) * RAD2DEG;

    // Розрахунок руху
    Vector3 movement = Vector3Scale(worldMoveDir, m_walkSpeed * deltaTime);
    movement = ClampMovementPerFrame(movement, 0.5f);

    Vector3 currentPos = GetPosition();
    Vector3 targetPos = Vector3Add(currentPos, movement);

    // Попередня перевірка колізій
    if (!m_lastCollisionManager) {
        TraceLog(LOG_ERROR, "🚨 Cannot check collision - no collision manager reference!");
        return;
    }

    SetPosition(targetPos);
    m_player->UpdatePlayerBox();

    Vector3 response = {};
    if (m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response)) {

        // Перевірка stuck marker
        float maxAxis = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        if (maxAxis > 400.0f) {
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED (max axis %.2f, count %d)", maxAxis, m_stuckCounter);

            if (m_stuckCounter >= 3) {
                SetPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f,0.0f,0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0;
                return;
            }

            if (!ExtractFromCollider()) {
                SetPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f,0.0f,0.0f});
                m_physics.SetGroundLevel(false);
            }
            return;
        }

        // Step-up
        if (TryStepUp(targetPos, response)) return;

        // Wall sliding
        WallSlide(currentPos, movement, response);
    }
    else {
        // Успішний рух без колізій
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();
    }
}

void PlayerMovement::ApplyAirborneMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    // In air: apply reduced air control
    const float AIR_CONTROL_FACTOR = 5.0f; // Reduced air control for more realistic physics
    Vector3 airMovement = Vector3Scale(worldMoveDir, AIR_CONTROL_FACTOR * deltaTime);
    
    // Get current velocity and apply air movement
    Vector3 currentVel = m_physics.GetVelocity();
    currentVel.x += airMovement.x;
    currentVel.z += airMovement.z;
    
    // Apply air resistance to horizontal movement
    const float AIR_RESISTANCE = 0.98f;
    currentVel.x *= AIR_RESISTANCE;
    currentVel.z *= AIR_RESISTANCE;
    
    m_physics.SetVelocity(currentVel);
}

bool PlayerMovement::ExtractFromCollider()
{
    // Check if we have a valid collision manager reference
    if (!m_lastCollisionManager) {
        TraceLog(LOG_ERROR, "🚨 Cannot extract player - no collision manager reference!");
        return false;
    }
    
    Vector3 currentPos = GetPosition();
    m_player->UpdatePlayerBox();
    
    Vector3 response = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response)) {
        TraceLog(LOG_INFO, "🔧 No collision detected - player is free");
        return false; // No collision, nothing to extract from
    }
    
    TraceLog(LOG_WARNING, "🚨 EXTRACTING PLAYER FROM COLLIDER - current pos: (%.2f, %.2f, %.2f)", 
             currentPos.x, currentPos.y, currentPos.z);
    
    // Try even more gentle extraction positions when stuck
    // Further reduced distances to prevent extreme teleportation
    Vector3 safePositions[] = {
        // First try very small adjustments near current position
        {currentPos.x, currentPos.y + 0.2f, currentPos.z}, // Tiny bit above
        {currentPos.x + 0.2f, currentPos.y, currentPos.z}, // Tiny bit right
        {currentPos.x - 0.2f, currentPos.y, currentPos.z}, // Tiny bit left
        {currentPos.x, currentPos.y, currentPos.z + 0.2f}, // Tiny bit forward
        {currentPos.x, currentPos.y, currentPos.z - 0.2f}, // Tiny bit back
        
        // Then try small adjustments
        {currentPos.x, currentPos.y + 0.4f, currentPos.z}, // Slightly above
        {currentPos.x + 0.4f, currentPos.y, currentPos.z}, // Slightly right
        {currentPos.x - 0.4f, currentPos.y, currentPos.z}, // Slightly left
        {currentPos.x, currentPos.y, currentPos.z + 0.4f}, // Slightly forward
        {currentPos.x, currentPos.y, currentPos.z - 0.4f}, // Slightly back
        
        // Then try medium adjustments
        {currentPos.x, currentPos.y + 0.8f, currentPos.z}, // Above
        {currentPos.x + 0.8f, currentPos.y + 0.2f, currentPos.z}, // Right and up
        {currentPos.x - 0.8f, currentPos.y + 0.2f, currentPos.z}, // Left and up
        {currentPos.x, currentPos.y + 0.2f, currentPos.z + 0.8f}, // Forward and up
        {currentPos.x, currentPos.y + 0.2f, currentPos.z - 0.8f}, // Back and up
        
        // Then try larger adjustments
        {currentPos.x, currentPos.y + 1.5f, currentPos.z}, // Higher above
        {currentPos.x + 1.5f, currentPos.y + 0.5f, currentPos.z}, // Far right and up
        {currentPos.x - 1.5f, currentPos.y + 0.5f, currentPos.z}, // Far left and up
        {currentPos.x, currentPos.y + 0.5f, currentPos.z + 1.5f}, // Far forward and up
        {currentPos.x, currentPos.y + 0.5f, currentPos.z - 1.5f}, // Far back and up
        
        // Finally try spawn positions
        {0.0f, 2.0f, 0.0f}, // Default spawn position
        {0.0f, 3.0f, 0.0f}, // Elevated spawn position
        {0.0f, 4.0f, 0.0f}, // High spawn position
        {0.0f, 5.0f, 0.0f}  // Very high spawn position
    };
    
    for (int i = 0; i < sizeof(safePositions)/sizeof(safePositions[0]); i++) {
        Vector3 safePos = safePositions[i];
        SetPosition(safePos);
        m_player->UpdatePlayerBox();
        Vector3 testResponse = {};
        if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), testResponse)) {
            TraceLog(LOG_INFO, "🏠 Found safe position [%d]: (%.2f, %.2f, %.2f)", i, safePos.x, safePos.y, safePos.z);
            m_physics.SetVelocity({0.0f, 0.0f, 0.0f}); // Stop all movement
            m_physics.SetGroundLevel(false); // Reset ground state
            return true;
        } else {
            TraceLog(LOG_WARNING, "❌ Safe position [%d] failed: (%.2f, %.2f, %.2f)", i, safePos.x, safePos.y, safePos.z);
        }
    }
    
    // If all safe positions failed, force to spawn position anyway
    SetPosition({0.0f, 2.0f, 0.0f});
    m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
    m_physics.SetGroundLevel(false);
    TraceLog(LOG_ERROR, "🚨 ALL SAFE POSITIONS FAILED - forced teleport to spawn");
    return true;
}