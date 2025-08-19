// Created by I#Oleg
#include <Player/Player.h>
#include <memory>
#include <raylib.h>

// ==================== CONSTANTS DEFINITIONS ====================

// Player constants - spawn above the ground plane center
const Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 2.0f, 0.0f}; // Spawn slightly above ground
const float Player::MODEL_Y_OFFSET = -1.2f;
const float Player::MODEL_SCALE = 1.0f;

// Constructor initializes default player parameters
Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    m_originalCameraTarget = m_cameraController->GetCamera().target;
    m_baseTarget = m_originalCameraTarget;

    m_playerPosition = DEFAULT_SPAWN_POSITION; // Use default spawn position
    // Depends on model size(bounding box collision)
    m_playerSize = {1.0f, 3.5f, 1.0f};
    m_playerColor = BLUE;
    m_playerModel = nullptr;
    m_useModel = false;
    
    // Initialize physics state
    m_physics.SetGroundLevel(false); // Start in air
    m_physics.SetVelocity({0, 0, 0}); // No initial velocity
    
    // Initialize collision
    UpdatePlayerBox();
    m_collision.Update(m_playerPosition, m_playerSize);
    
    // Initialize collision manager reference
    m_lastCollisionManager = nullptr;
}

Player::~Player() = default;

// Main update function called every frame
void Player::Update()
{
    ApplyInput();
    m_cameraController->UpdateCameraRotation();
    m_cameraController->UpdateMouseRotation(m_cameraController->GetCamera(), m_playerPosition);
    m_cameraController->Update();
    UpdatePlayerBox();
}

float Player::GetSpeed() { return m_walkSpeed; }

float Player::GetRotationY() const { return m_rotationY; }

void Player::SetSpeed(const float speed) { this->m_walkSpeed = speed; }

void Player::Move(const Vector3 &moveVector)
{
    m_playerPosition = Vector3Add(m_playerPosition, moveVector);
}

// Handle input both on ground and mid-air
void Player::ApplyInput()
{
    float deltaTime = GetFrameTime();
    m_physics.Update(deltaTime);

    Vector3 inputDirection = GetInputDirection();
    if (Vector3Length(inputDirection) < 0.001f) return;

    inputDirection = Vector3Normalize(inputDirection);
    auto [forward, right] = GetCameraVectors();

    // Проєкція на горизонтальну площину
    forward.y = 0.0f;
    right.y = 0.0f;
    forward = Vector3Normalize(forward);
    right = Vector3Normalize(right);

    Vector3 worldMoveDir = {right.x * inputDirection.x + forward.x * inputDirection.z, 0.0f,
                            right.z * inputDirection.x + forward.z * inputDirection.z};

    if (Vector3Length(worldMoveDir) > 0.001f)
        worldMoveDir = Vector3Normalize(worldMoveDir);

    if (m_physics.IsGrounded())
        ApplyGroundedMovement(worldMoveDir, deltaTime);
    else
        ApplyAirborneMovement(worldMoveDir, deltaTime);
}

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }
Models &Player::GetModelManager() { return m_modelPlayer; }

// Update the bounding box based on player position and size
void Player::UpdatePlayerBox()
{
    m_playerBoundingBox.min = Vector3Subtract(m_playerPosition, Vector3Scale(m_playerSize, 0.5f));
    m_playerBoundingBox.max = Vector3Add(m_playerPosition, Vector3Scale(m_playerSize, 0.5f));
}

void Player::UpdatePlayerCollision() { m_collision.Update(m_playerPosition, m_playerSize); }

void Player::SetPlayerModel(Model *model) { m_playerModel = model; }
void Player::ToggleModelRendering(bool useModel) { m_useModel = useModel; }

void Player::SetPlayerPosition(const Vector3 &pos)
{
    m_playerPosition = pos;
    UpdatePlayerBox();
    m_collision.Update(m_playerPosition, m_playerSize);
}

const Collision &Player::GetCollision() const { return m_collision; }

bool Player::IsJumpCollision() const { return m_isJumpCollision; }
Vector3 Player::GetPlayerPosition() const { return m_playerPosition; }

// Apply jump impulse based on mass and direction
void Player::ApplyJumpImpulse(float impulse)
{
    if (!m_physics.IsGrounded())
        return;

    // Calculate jump velocity
    float mass = m_playerSize.x * m_playerSize.y * m_playerSize.z * 1.0f;
    float verticalVelocity = impulse / mass;

    // Get current horizontal velocity to preserve momentum
    Vector3 currentVel = m_physics.GetVelocity();
    
    // Set jump velocity (preserve horizontal movement, add vertical impulse)
    Vector3 jumpVelocity = {currentVel.x, verticalVelocity, currentVel.z};
    
    m_physics.SetVelocity(jumpVelocity);
    m_physics.SetGroundLevel(false);
    m_isJumping = true;
    m_jumpStartTime = GetTime();
}

void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    float deltaTime = GetFrameTime();

    HandleJumpInput();
    HandleEmergencyReset();

    ApplyGravity(deltaTime);

    Vector3 newPosition = StepMovement(collisionManager);

    SetPlayerPosition(newPosition);
    UpdatePlayerBox();

    SnapToGroundIfNeeded(collisionManager);
}

void Player::HandleJumpInput()
{
    if (IsKeyDown(KEY_SPACE) && m_physics.IsGrounded())
    {
        ApplyJumpImpulse(m_physics.GetJumpStrength() * 8.0f);
        m_isJumping = true;
        m_physics.SetGroundLevel(false);
    }
}

void Player::HandleEmergencyReset()
{
    if (IsKeyPressed(KEY_T))
    {
        SetPlayerPosition(DEFAULT_SPAWN_POSITION);
        m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
        m_physics.SetGroundLevel(false);
        m_isJumping = false;
    }
}

void Player::ApplyGravity(float deltaTime)
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

Vector3 Player::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 move = Vector3Scale(m_physics.GetVelocity(), GetFrameTime());
    Vector3 newPosition = GetPlayerPosition();

    float moveDistance = Vector3Length(move);
    if (moveDistance < 0.001f) return newPosition;

    int steps = (int)(moveDistance / 0.005f) + 1;
    steps = steps > 200 ? 200 : steps;
    Vector3 stepMove = Vector3Scale(move, 1.0f / steps);

    for (int i = 0; i < steps; i++)
    {
        Vector3 testPos = Vector3Add(newPosition, stepMove);
        SetPlayerPosition(testPos);
        UpdatePlayerBox();

        Vector3 response;
        if (collisionManager.CheckCollision(GetCollision(), response))
        {
            ResolveCollision(response);
            newPosition = GetPlayerPosition();
            break;
        }
        else
        {
            newPosition = testPos;
        }
    }

    return newPosition;
}

void Player::ResolveCollision(const Vector3 &response)
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
            Vector3 pos = GetPlayerPosition();
            pos.y += response.y + 0.05f;
            SetPlayerPosition(pos);
            m_physics.SetGroundLevel(true);
            velocity.y = 0.0f;
            m_isJumping = false;
        }
        else // Ceiling
        {
            Vector3 pos = GetPlayerPosition();
            pos.y += response.y;
            SetPlayerPosition(pos);
            velocity.y = 0.0f;
        }
    }
    else // Wall collision
    {
        Vector3 pos = GetPlayerPosition();
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
        SetPlayerPosition(pos);
    }

    m_physics.SetVelocity(velocity);
}

void Player::SnapToGroundIfNeeded(const CollisionManager &collisionManager)
{
    Vector3 pos = GetPlayerPosition();
    float groundTop = PhysicsComponent::GROUND_COLLISION_CENTER.y +
                      PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;

    // Simple snap to world floor
    if (!m_physics.IsGrounded() && pos.y <= groundTop + 0.1f)
    {
        pos.y = groundTop + 0.05f;
        SetPlayerPosition(pos);
        Vector3 vel = m_physics.GetVelocity();
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);
        m_physics.SetGroundLevel(true);
        m_isJumping = false;
    }

    // Optional: extra thin-floor detection via raycast around player
    if (!m_physics.IsGrounded() && !m_isJumping)
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

            SetPlayerPosition(testPos);
            UpdatePlayerBox();

            Vector3 floorResponse;
            if (collisionManager.CheckCollision(GetCollision(), floorResponse) && floorResponse.y > 0.01f)
            {
                float maxH = fmaxf(fabsf(floorResponse.x), fabsf(floorResponse.z));
                if (floorResponse.y > maxH * 0.8f)
                {
                    pos.y = testPos.y + floorResponse.y + 0.15f;
                    SetPlayerPosition(pos);
                    Vector3 vel = m_physics.GetVelocity();
                    vel.y = 0.0f;
                    m_physics.SetVelocity(vel);
                    m_physics.SetGroundLevel(true);
                    m_isJumping = false;
                    break;
                }
            }
        }
    }
}

BoundingBox Player::GetPlayerBoundingBox() const // Get bounding box
{
    return m_playerBoundingBox;
}

const PhysicsComponent &Player::GetPhysics() const // Get physics component
{
    return m_physics;
}

PhysicsComponent &Player::GetPhysics() // Get physics component (non-const)
{
    return m_physics;
}

// Helper methods implementation
Vector3 Player::GetInputDirection()
{
    Vector3 inputDir = {};

    if (IsKeyDown(KEY_W))
        inputDir.z -= 1.0f;
    if (IsKeyDown(KEY_S))
        inputDir.z += 1.0f;
    if (IsKeyDown(KEY_A))
        inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        inputDir.x += 1.0f;

    // Update speed based on sprint key
    m_walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? m_runSpeed : 3.1f;

    return inputDir;
}

std::pair<Vector3, Vector3> Player::GetCameraVectors() const {
    const Camera &camera = m_cameraController->GetCamera();

    Vector3 forward = Vector3Subtract(camera.position, camera.target);
    forward.y = 0;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, forward));

    return {forward, right};
}

Vector3 Player::ClampMovementPerFrame(const Vector3 &movement, float maxMove)
{
    float len = Vector3Length(movement);
    if (len > maxMove) {
        TraceLog(LOG_INFO, "🚀 Movement clamped from %.3f to %.3f units", len, maxMove);
        return Vector3Scale(Vector3Normalize(movement), maxMove);
    }
    return movement;
}

// Спроба піднятися на невеликий крок
bool Player::TryStepUp(const Vector3 &targetPos, const Vector3 &response)
{
    const float MAX_STEP_HEIGHT = 0.3f;
    if (!m_physics.IsGrounded() || response.y <= 0.01f || response.y >= MAX_STEP_HEIGHT)
        return false;

    Vector3 stepUpPos = targetPos;
    stepUpPos.y += response.y + 0.05f;
    SetPlayerPosition(stepUpPos);
    UpdatePlayerBox();

    Vector3 stepResp = {};
    if (!m_lastCollisionManager->CheckCollision(GetCollision(), stepResp)) {
        TraceLog(LOG_INFO, "🪜 Step up successful, height: %.2f", response.y);
        return true;
    }

    SetPlayerPosition(targetPos); // відкат якщо не вдалося
    UpdatePlayerBox();
    return false;
}

// Sliding вздовж стіни
void Player::WallSlide(const Vector3 &currentPos, const Vector3 &movement, const Vector3 &response)
{
    Vector3 wallNormal = Vector3Normalize(response);
    Vector3 slideDir = Vector3Subtract(movement, Vector3Scale(wallNormal, Vector3DotProduct(movement, wallNormal)));
    slideDir = Vector3Scale(slideDir, 0.8f);

    Vector3 slidePos = Vector3Add(currentPos, slideDir);
    SetPlayerPosition(slidePos);
    UpdatePlayerBox();

    Vector3 slideResp = {};
    if (!m_lastCollisionManager->CheckCollision(GetCollision(), slideResp)) {
        TraceLog(LOG_INFO, "🚧 Wall sliding successful");
    } else {
        slidePos = Vector3Add(currentPos, Vector3Scale(response, 1.1f));
        SetPlayerPosition(slidePos);
        UpdatePlayerBox();
        TraceLog(LOG_INFO, "🚧 Wall sliding failed, applied collision response");
    }
}

// ================= MAIN MOVEMENT FUNCTION ===================

void Player::ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    if (Vector3Length(worldMoveDir) < 0.001f) return;

    // Поворот гравця
    m_rotationY = atan2f(worldMoveDir.x, worldMoveDir.z) * RAD2DEG;

    // Розрахунок руху
    Vector3 movement = Vector3Scale(worldMoveDir, m_walkSpeed * deltaTime);
    movement = ClampMovementPerFrame(movement, 0.5f);

    Vector3 currentPos = GetPlayerPosition();
    Vector3 targetPos = Vector3Add(currentPos, movement);

    // Попередня перевірка колізій
    if (!m_lastCollisionManager) {
        TraceLog(LOG_ERROR, "🚨 Cannot check collision - no collision manager reference!");
        return;
    }

    SetPlayerPosition(targetPos);
    UpdatePlayerBox();

    Vector3 response = {};
    if (m_lastCollisionManager->CheckCollision(GetCollision(), response)) {

        // Перевірка stuck marker
        float maxAxis = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        if (maxAxis > 400.0f) {
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED (max axis %.2f, count %d)", maxAxis, m_stuckCounter);

            if (m_stuckCounter >= 3) {
                SetPlayerPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f,0.0f,0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0;
                return;
            }

            if (!ExtractFromCollider()) {
                SetPlayerPosition({0.0f, 15.0f, 0.0f});
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
        SetPlayerPosition(targetPos);
        UpdatePlayerBox();
    }
}

void Player::ApplyAirborneMovement(const Vector3 &worldMoveDir, float deltaTime)
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

bool Player::ExtractFromCollider()
{
    // Check if we have a valid collision manager reference
    if (!m_lastCollisionManager) {
        TraceLog(LOG_ERROR, "🚨 Cannot extract player - no collision manager reference!");
        return false;
    }
    
    Vector3 currentPos = GetPlayerPosition();
    UpdatePlayerBox();
    
    Vector3 response = {};
    if (!m_lastCollisionManager->CheckCollision(GetCollision(), response)) {
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
        SetPlayerPosition(safePos);
        UpdatePlayerBox();
        Vector3 testResponse = {};
        if (!m_lastCollisionManager->CheckCollision(GetCollision(), testResponse)) {
            TraceLog(LOG_INFO, "🏠 Found safe position [%d]: (%.2f, %.2f, %.2f)", i, safePos.x, safePos.y, safePos.z);
            m_physics.SetVelocity({0.0f, 0.0f, 0.0f}); // Stop all movement
            m_physics.SetGroundLevel(false); // Reset ground state
            return true;
        } else {
            TraceLog(LOG_WARNING, "❌ Safe position [%d] failed: (%.2f, %.2f, %.2f)", i, safePos.x, safePos.y, safePos.z);
        }
    }
    
    // If all safe positions failed, force to spawn position anyway
    SetPlayerPosition({0.0f, 2.0f, 0.0f});
    m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
    m_physics.SetGroundLevel(false);
    TraceLog(LOG_ERROR, "🚨 ALL SAFE POSITIONS FAILED - forced teleport to spawn");
    return true;
}