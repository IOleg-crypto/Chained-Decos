#include "raylib.h"
#include <Player/Player.h>
#include <Player/PlayerMovement.h>

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
        return;
    }

    TraceLog(LOG_INFO, "Jump impulse received: %.2f", impulse);

    // Calculate jump velocity
    float mass = 1.0f;
    float verticalVelocity = impulse / mass;

    // Get forward direction from camera
    Vector3 forward = Vector3Subtract(m_player->GetCameraController()->GetCamera().target,
                                      m_player->GetCameraController()->GetCamera().position);
    forward.y = 0;
    forward = Vector3Normalize(forward);

    // Calculate horizontal velocity based on forward direction
    Vector3 horizontalVelocity = Vector3Scale(forward, m_walkSpeed);

    // Combine vertical and horizontal components
    Vector3 jumpVelocity = {horizontalVelocity.x, verticalVelocity, horizontalVelocity.z};

    // Apply the velocity
    m_physics.SetVelocity(jumpVelocity);

    TraceLog(LOG_INFO, "Jump impulse applied: (%.2f, %.2f, %.2f)", jumpVelocity.x, jumpVelocity.y,
             jumpVelocity.z);

    // Set player as not grounded
    m_physics.SetGroundLevel(false);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * deltaTime;

        // const float MAX_FALL_SPEED = -20.0f;
        // if (vel.y < MAX_FALL_SPEED)
        //     vel.y = MAX_FALL_SPEED;

        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    Vector3 move = Vector3Scale(velocity, GetFrameTime());
    Vector3 newPosition = GetPosition();

    float moveDistance = Vector3Length(move);
    if (moveDistance < 0.001f)
    {
        return newPosition;
    }

    // Limit maximum movement per frame to prevent tunneling through objects
    const float MAX_MOVE_PER_FRAME = 0.5f;
    if (moveDistance > MAX_MOVE_PER_FRAME)
    {
        move = Vector3Scale(Vector3Normalize(move), MAX_MOVE_PER_FRAME);
        moveDistance = MAX_MOVE_PER_FRAME;
    }

    // Calculate steps based on distance
    int steps = (int)(moveDistance / 0.005f) + 1;
    steps = steps > 200 ? 200 : steps;
    Vector3 stepMove = Vector3Scale(move, 1.0f / steps);

    bool collisionDetected = false;
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
            collisionDetected = true;
        }
        else
        {
            newPosition = testPos;
        }
    }

    // If we didn't hit anything but we're moving very slowly, apply additional friction
    if (!collisionDetected && moveDistance < 0.05f)
    {
        // Apply additional friction to very slow movements to prevent sliding
        Vector3 currentVel = m_physics.GetVelocity();
        const float EXTRA_FRICTION = 0.7f;
        currentVel.x *= EXTRA_FRICTION;
        currentVel.z *= EXTRA_FRICTION;

        // Zero out very small velocities
        const float STOP_THRESHOLD = 0.01f;
        if (fabsf(currentVel.x) < STOP_THRESHOLD)
        {
            currentVel.x = 0.f;
        }
        if (fabsf(currentVel.z) < STOP_THRESHOLD)
        {
            currentVel.z = 0.0f;
        }

        m_physics.SetVelocity(currentVel);
    }

    return newPosition;
}

void PlayerMovement::ResolveCollision(const Vector3 &response)
{
    Vector3 velocity = m_physics.GetVelocity();
    float absX = fabsf(response.x);
    float absY = fabsf(response.y);
    float absZ = fabsf(response.z);

    // Calculate response magnitude for logging
    float responseLength = Vector3Length(response);
    TraceLog(LOG_INFO, "Collision detected, applying response: (%.2f, %.2f, %.2f)", response.x,
             response.y, response.z);

    // Floor/ceiling collision
    if (absY >= absX && absY >= absZ)
    {
        if (response.y > 0.0f) // Floor
        {
            Vector3 pos = GetPosition();
            pos.y += response.y + 0.01f;
            SetPosition(pos);
            m_physics.SetGroundLevel(true);

            // Zero out vertical velocity
            velocity.y = 0.0f;

            // Apply friction to horizontal movement when hitting floor
            const float GROUND_FRICTION = 0.8f;
            velocity.x *= GROUND_FRICTION;
            velocity.z *= GROUND_FRICTION;

            TraceLog(LOG_INFO, "Floor collision resolved, new Y: %.3f, offset: %.3f", pos.y,
                     response.y + 0.01f);
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

            // Apply friction to other components
            const float WALL_FRICTION = 0.7f;
            velocity.z *= WALL_FRICTION;
        }
        else
        {
            pos.z += response.z;
            velocity.z = 0.0f;

            // Apply friction to other components
            const float WALL_FRICTION = 0.7f;
            velocity.x *= WALL_FRICTION;
        }
        SetPosition(pos);
    }

    // Apply a small velocity threshold to prevent micro-movements
    const float VELOCITY_THRESHOLD = 0.05f;
    if (fabsf(velocity.x) < VELOCITY_THRESHOLD)
        velocity.x = 0.0f;
    if (fabsf(velocity.z) < VELOCITY_THRESHOLD)
        velocity.z = 0.0f;

    // Log final velocity after adjustments
    TraceLog(LOG_INFO, "Final response vector: (%.3f, %.3f, %.3f) length=%.3f", response.x,
             response.y, response.z, responseLength);

    m_physics.SetVelocity(velocity);
}

void PlayerMovement::SnapToGroundIfNeeded(const CollisionManager &collisionManager)
{
    // Don't snap to ground if we're in the middle of a jump with upward velocity
    Vector3 vel = m_physics.GetVelocity();
    // if (vel.y > 0.1f)
    // {
    //     return;
    // }

    Vector3 pos = GetPosition();
    float groundTop = PhysicsComponent::GROUND_COLLISION_CENTER.y +
                      PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;

    TraceLog(LOG_DEBUG, "Ground top: %.3f", groundTop);

    // Only snap to world floor if we're below a certain height and not already grounded
    if (!m_physics.IsGrounded() && pos.y <= groundTop + 0.5f)
    {
        // Check if we're falling (negative y velocity)
        if (vel.y < 0.0f)
        {
            pos.y = groundTop + 0.01f;
            SetPosition(pos);
            vel.y = 0.0f;
            m_physics.SetVelocity(vel);
            m_physics.SetGroundLevel(true);

            TraceLog(LOG_INFO, "Snapped to ground, new Y: %.3f", pos.y);
        }
    }

    // Optional: extra thin-floor detection via raycast around player
    if (!m_physics.IsGrounded() && vel.y <= 0.0f)
    {
        const float RAY_DIST = 0.2f;
        Vector3 offsets[] = {{0, 0, 0},        {0.3f, 0, 0},     {-0.3f, 0, 0},
                             {0, 0, 0.3f},     {0, 0, -0.3f},    {0.2f, 0, 0.2f},
                             {-0.2f, 0, 0.2f}, {0.2f, 0, -0.2f}, {-0.2f, 0, -0.2f}};

        for (Vector3 off : offsets)
        {
            Vector3 testPos = pos;
            testPos.x += off.x;
            testPos.z += off.z;
            testPos.y -= RAY_DIST;

            SetPosition(testPos);
            m_player->UpdatePlayerBox();

            Vector3 floorResponse;
            if (collisionManager.CheckCollision(m_player->GetCollision(), floorResponse) &&
                floorResponse.y > 0.01f)
            {
                float maxH = fmaxf(fabsf(floorResponse.x), fabsf(floorResponse.z));
                if (floorResponse.y > maxH * 0.8f)
                {
                    pos.y = testPos.y + floorResponse.y + 0.01f;
                    SetPosition(pos);
                    vel.y = 0.0f;
                    m_physics.SetVelocity(vel);
                    m_physics.SetGroundLevel(true);

                    TraceLog(LOG_INFO, "Thin floor detected, new Y: %.3f, response: %.3f", pos.y,
                             floorResponse.y);
                    break;
                }
            }
        }
    }
}

Vector3 PlayerMovement::ClampMovementPerFrame(const Vector3 &movement, float maxMove)
{
    float len = Vector3Length(movement);
    if (len > maxMove)
    {
        TraceLog(LOG_INFO, "🚀 Movement clamped from %.3f to %.3f units", len, maxMove);
        return Vector3Scale(Vector3Normalize(movement), maxMove);
    }
    return movement;
}

bool PlayerMovement::TryStepUp(const Vector3 &targetPos, const Vector3 &response)
{
    const float MAX_STEP_HEIGHT = 0.5f;
    if (!m_physics.IsGrounded() || response.y <= 0.01f || response.y >= MAX_STEP_HEIGHT)
        TraceLog(LOG_INFO, "🚫 Not attempting step up");
    return false;

    Vector3 stepUpPos = targetPos;
    stepUpPos.y += response.y + 0.01f;
    SetPosition(stepUpPos);
    m_player->UpdatePlayerBox();

    TraceLog(LOG_INFO, "Attempting step up, height: %.3f, offset: %.3f", response.y,
             response.y + 0.01f);

    Vector3 stepResp = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), stepResp))
    {
        TraceLog(LOG_INFO, "🪜 Step up successful, height: %.2f", response.y);
        return true;
    }

    SetPosition(targetPos);
    m_player->UpdatePlayerBox();
    return false;
}

void PlayerMovement::WallSlide(const Vector3 &currentPos, const Vector3 &movement,
                               const Vector3 &response)
{
    Vector3 wallNormal = Vector3Normalize(response);
    Vector3 slideDir = Vector3Subtract(
        movement, Vector3Scale(wallNormal, Vector3DotProduct(movement, wallNormal)));

    // Reduce slide speed more significantly
    slideDir = Vector3Scale(slideDir, 0.6f);

    Vector3 slidePos = Vector3Add(currentPos, slideDir);
    SetPosition(slidePos);
    m_player->UpdatePlayerBox();

    Vector3 slideResp = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), slideResp))
    {
        TraceLog(LOG_INFO, "🚧 Wall sliding successful");

        // Adjust velocity to match the slide direction but with reduced magnitude
        Vector3 velocity = m_physics.GetVelocity();
        Vector3 slideVelocity =
            Vector3Scale(Vector3Normalize(slideDir), Vector3Length(velocity) * 0.6f);

        // Apply a small velocity threshold to prevent micro-movements
        const float VELOCITY_THRESHOLD = 0.1f;
        if (fabsf(slideVelocity.x) < VELOCITY_THRESHOLD)
            slideVelocity.x = 0.0f;
        if (fabsf(slideVelocity.z) < VELOCITY_THRESHOLD)
            slideVelocity.z = 0.0f;

        // Keep vertical velocity component
        slideVelocity.y = velocity.y;

        m_physics.SetVelocity(slideVelocity);
    }
    else
    {
        slidePos = Vector3Add(currentPos, Vector3Scale(response, 1.1f));
        SetPosition(slidePos);
        m_player->UpdatePlayerBox();
        TraceLog(LOG_INFO, "🚧 Wall sliding failed, applied collision response");

        // When wall sliding fails, reduce velocity more aggressively
        Vector3 velocity = m_physics.GetVelocity();
        velocity.x *= 0.3f;
        velocity.z *= 0.3f;

        // Apply a higher velocity threshold
        const float VELOCITY_THRESHOLD = 0.2f;
        if (fabsf(velocity.x) < VELOCITY_THRESHOLD)
            velocity.x = 0.0f;
        if (fabsf(velocity.z) < VELOCITY_THRESHOLD)
            velocity.z = 0.0f;

        m_physics.SetVelocity(velocity);
    }
}

void PlayerMovement::ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    if (Vector3Length(worldMoveDir) < 0.001f)
        TraceLog(LOG_INFO, "No movement this frame");
    return;

    m_rotationY = atan2f(worldMoveDir.x, worldMoveDir.z) * RAD2DEG;

    Vector3 movement = Vector3Scale(worldMoveDir, m_walkSpeed * deltaTime);
    movement = ClampMovementPerFrame(movement, 0.5f);

    Vector3 currentPos = GetPosition();
    Vector3 targetPos = Vector3Add(currentPos, movement);

    if (!m_lastCollisionManager)
    {
        TraceLog(LOG_ERROR, "🚨 Cannot check collision - no collision manager reference!");
        return;
    }

    SetPosition(targetPos);
    m_player->UpdatePlayerBox();

    Vector3 response = {};
    if (m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
    {

        float maxAxis = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        if (maxAxis > 400.0f)
        {
            TraceLog(LOG_ERROR, "🚨 Called maxaxis (max axis %.2f)", maxAxis);
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED (max axis %.2f, count %d)", maxAxis,
                     m_stuckCounter);

            if (m_stuckCounter >= 3)
            {
                SetPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0;
                return;
            }

            if (!ExtractFromCollider())
            {
                SetPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
            }
            return;
        }

        // Step-up
        if (TryStepUp(targetPos, response))
            return;

        // Wall sliding
        WallSlide(currentPos, movement, response);
    }
    else
    {
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
    if (!m_lastCollisionManager)
    {
        TraceLog(LOG_ERROR, "🚨 Cannot extract player - no collision manager reference!");
        return false;
    }

    Vector3 currentPos = GetPosition();
    m_player->UpdatePlayerBox();

    Vector3 response = {};
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
    {
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
        {currentPos.x, currentPos.y + 0.8f, currentPos.z},        // Above
        {currentPos.x + 0.8f, currentPos.y + 0.2f, currentPos.z}, // Right and up
        {currentPos.x - 0.8f, currentPos.y + 0.2f, currentPos.z}, // Left and up
        {currentPos.x, currentPos.y + 0.2f, currentPos.z + 0.8f}, // Forward and up
        {currentPos.x, currentPos.y + 0.2f, currentPos.z - 0.8f}, // Back and up

        // Then try larger adjustments
        {currentPos.x, currentPos.y + 1.5f, currentPos.z},        // Higher above
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

    for (size_t i = 0; i < sizeof(safePositions) / sizeof(safePositions[0]); i++)
    {
        Vector3 safePos = safePositions[i];
        SetPosition(safePos);
        m_player->UpdatePlayerBox();
        Vector3 testResponse = {};
        if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), testResponse))
        {
            TraceLog(LOG_INFO, "🏠 Found safe position [%d]: (%.2f, %.2f, %.2f)", i, safePos.x,
                     safePos.y, safePos.z);
            m_physics.SetVelocity({0.0f, 0.0f, 0.0f}); // Stop all movement
            m_physics.SetGroundLevel(false);           // Reset ground state
            return true;
        }
        TraceLog(LOG_WARNING, "❌ Safe position [%d] failed: (%.2f, %.2f, %.2f)", i, safePos.x,
                 safePos.y, safePos.z);
    }

    // If all safe positions failed, force to spawn position anyway
    SetPosition({0.0F, 2.0F, 0.0F});
    m_physics.SetVelocity({0.0F, 0.0F, 0.0F});
    m_physics.SetGroundLevel(false);
    TraceLog(LOG_ERROR, "🚨 ALL SAFE POSITIONS FAILED - forced teleport to spawn");
    return true;
}