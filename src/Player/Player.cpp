// Created by I#Oleg
#include <Player/Player.h>
#include <memory>
#include <raylib.h>

// ==================== CONSTANTS DEFINITIONS ====================

// Player constants - spawn above the ground plane center
const Vector3 Player::DEFAULT_SPAWN_POSITION = {
    0.0f, 0.0f, 0.0f}; // Higher spawn to avoid ground collision issues
const float Player::MODEL_Y_OFFSET = -1.2f;
const float Player::MODEL_SCALE = 1.0f;

// Constructor initializes default player parameters
Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    m_originalCameraTarget = m_cameraController->GetCamera().target;
    m_baseTarget = m_originalCameraTarget;

    m_playerPosition = {3.0f, 5.0f, 0.0f}; // Start well above ground plane center
    // Depends on model size(bounding box collision)
    m_playerSize = {1.0f, 3.5f, 1.0f};
    m_playerColor = BLUE;
    m_playerModel = nullptr;
    m_useModel = false;
    UpdatePlayerBox();
}

Player::~Player() = default;

// Main update function called every frame
void Player::Update()
{
    m_cameraController->UpdateCameraRotation();
    m_cameraController->UpdateMouseRotation(m_cameraController->GetCamera(), m_playerPosition);
    m_cameraController->Update();
    ApplyInput();
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
    if (Vector3Length(inputDirection) == 0)
        return; // No input

    // Normalize input and get camera vectors
    inputDirection = Vector3Normalize(inputDirection);
    auto [forward, right] = GetCameraVectors();

    // Calculate world movement direction
    Vector3 worldMoveDir = {right.x * inputDirection.x + forward.x * inputDirection.z, 0.0f,
                            right.z * inputDirection.x + forward.z * inputDirection.z};

    // Apply movement based on grounded state
    if (m_physics.IsGrounded())
    {
        ApplyGroundedMovement(worldMoveDir, deltaTime);
    }
    else
    {
        ApplyAirborneMovement(worldMoveDir, deltaTime);
    }
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
    
    // Handle jumping
    if (IsKeyDown(KEY_SPACE) && m_physics.IsGrounded())
    {
        ApplyJumpImpulse(m_physics.GetJumpStrength() * 8.0f);
    }
    
    // Emergency reset to spawn (T key)
    if (IsKeyPressed(KEY_T))
    {
        TraceLog(LOG_WARNING, "🚨 EMERGENCY RESET - teleporting to spawn");
        SetPlayerPosition(DEFAULT_SPAWN_POSITION);
        m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
        m_physics.SetGroundLevel(false);
    }

    // Get current position
    Vector3 playerPosition = GetPlayerPosition();
    
    // Apply gravity and physics only if not grounded
    if (!m_physics.IsGrounded())
    {
        // Apply gravity
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * deltaTime;
        m_physics.SetVelocity(vel);

        // Apply velocity to position
        Vector3 move = Vector3Scale(vel, deltaTime);
        playerPosition = Vector3Add(playerPosition, move);
        
        SetPlayerPosition(playerPosition);
    }

    // Update collision box before checking
    UpdatePlayerBox();

    // Check for collisions
    Vector3 response = {};
    bool isColliding = collisionManager.CheckCollision(GetCollision(), response);
    playerPosition = GetPlayerPosition(); // Get updated position

    if(isColliding)
    {
        // Визначаємо тип колізії за найбільшою віссю відповіді
        float absX = fabsf(response.x);
        float absY = fabsf(response.y);
        float absZ = fabsf(response.z);
        
        // Ігноруємо мікро-колізії
        if (absX < 0.01f && absY < 0.01f && absZ < 0.01f)
            return;
            
        // Перевіряємо, чи це колізія з підлогою (Y-вісь вгору)
        bool isFloorCollision = (absY > absX && absY > absZ && response.y > 0);
        
        if (isFloorCollision && !m_physics.IsGrounded())
        {
            // Колізія з підлогою коли гравець у повітрі - приземляємо його
            m_physics.SetGroundLevel(true);
            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
            m_isJumping = false;
            
            // Додаємо невеликий буфер, щоб запобігти повторній колізії
            playerPosition.y += response.y + 0.05f;
            SetPlayerPosition(playerPosition);
            TraceLog(LOG_INFO, "Приземлення на підлогу: y=%.2f", playerPosition.y);
        }
        else if (isFloorCollision && m_physics.IsGrounded())
        {
            // Колізія з підлогою коли гравець на землі - просто рухаємо вгору
            playerPosition.y += response.y;
            SetPlayerPosition(playerPosition);
        }
        else
        {
            // Стіна або стеля - застосовуємо відповідь колізії
            // Якщо гравець у повітрі, зберігаємо вертикальну швидкість
            Move(response);
            
            // Якщо це стіна (X або Z домінує), зменшуємо горизонтальну швидкість
            if (absX > absY || absZ > absY)
            {
                Vector3 vel = m_physics.GetVelocity();
                vel.x *= 0.5f;
                vel.z *= 0.5f;
                m_physics.SetVelocity(vel);
            }
            
            // Якщо це стеля (Y-вісь вниз), зупиняємо вертикальний рух вгору
            if (absY > absX && absY > absZ && response.y < 0)
            {
                Vector3 vel = m_physics.GetVelocity();
                if (vel.y > 0) vel.y = 0;
                m_physics.SetVelocity(vel);
            }
            
            UpdatePlayerBox();
        }
        
        // Перевірка на випадіння за межі світу
        if (playerPosition.y < -50.0f)
        {
            TraceLog(LOG_WARNING, "Гравець випав за межі світу! Телепортація на спавн.");
            SetPlayerPosition(DEFAULT_SPAWN_POSITION);
            m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
            m_physics.SetGroundLevel(false);
        }
    }
    else
    {
        // Немає колізії - перевіряємо, чи гравець над землею
        // Використовуємо просту перевірку висоти над рівнем землі
        float groundLevel = PhysicsComponent::GROUND_COLLISION_CENTER.y + 
                           PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;
        
        // Якщо гравець на землі, але знаходиться вище рівня землі - починаємо падіння
        if (m_physics.IsGrounded() && playerPosition.y > groundLevel + 0.5f)
        {
            m_physics.SetGroundLevel(false);
            TraceLog(LOG_INFO, "Гравець почав падіння з висоти: %.2f", playerPosition.y - groundLevel);
        }
        
        // Якщо гравець падає і досяг рівня землі - приземляємо його
        if (!m_physics.IsGrounded() && 
            m_physics.GetVelocityY() <= 0.0f && 
            playerPosition.y <= groundLevel + 0.1f)
        {
            playerPosition.y = groundLevel + 0.05f;
            SetPlayerPosition(playerPosition);
            m_physics.SetGroundLevel(true);
            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
            m_isJumping = false;
            TraceLog(LOG_INFO, "Приземлення на основну площину: y=%.2f", playerPosition.y);
        }
    }
    
    // Фінальна перевірка - якщо гравець на землі, вертикальна швидкість має бути нульовою
    if (m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        if (vel.y != 0.0f)
        {
            m_physics.SetVelocity({vel.x, 0.0f, vel.z});
        }
    }
    
    // if (isColliding)
    // {
    //     // Get maximum response axis
    //     float maxAxisResponse = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
            
    //     // Ignore micro-collisions
    //     const float MIN_RESPONSE_THRESHOLD = 0.1f;
    //     if (maxAxisResponse < MIN_RESPONSE_THRESHOLD)
    //     {
    //         return;
    //     }
        
    //     // // Check if falling fast
    //     // Vector3 velocity = m_physics.GetVelocity();
    //     // bool isFallingFast = velocity.y < -10.0f;
        
    //     // // Determine collision type based on response direction
    //     // float absX = fabsf(response.x);
    //     // float absY = fabsf(response.y);
    //     // float absZ = fabsf(response.z);

    //     // // Floor/ceiling collision (Y-axis dominant)
    //     // if (absY >= absX && absY >= absZ)
    //     // {
    //     //     if (response.y > 0.0f) // Floor collision
    //     //     {
    //     //         // Limit floor response
    //     //         float maxFloorResponse = isFallingFast ? 0.5f : 2.0f;
    //     //         float limitedYResponse = response.y > maxFloorResponse ? maxFloorResponse : response.y;
                
    //     //         // Check if it's an actual floor
    //     //         bool isActualFloor = (m_physics.GetVelocityY() <= 0.0f && !m_isJumping && 
    //     //                              absY > absX * 1.2f && absY > absZ * 1.2f);
                
    //     //         if (isActualFloor)
    //     //         {
    //     //             // Move player up to surface
    //     //             playerPosition.y += limitedYResponse;
                    
    //     //             // Set as grounded and stop vertical movement
    //     //             m_physics.SetGroundLevel(true);
    //     //             m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
    //     //             m_isJumping = false;
                    
    //     //             // Add small buffer to prevent immediate re-collision
    //     //             playerPosition.y += 0.05f; // Increased buffer to prevent floating
                    
    //     //             TraceLog(LOG_INFO, "Floor collision: Player grounded at y=%.2f", playerPosition.y);
    //     //         }
    //     //         else
    //     //         {
    //     //             // Not a clear floor - just apply collision response without grounding
    //     //             playerPosition.y += limitedYResponse;
    //     //         }
    //     //     }
    //     //     else // Ceiling collision
    //     //     {
    //     //         // Limit ceiling response
    //     //         float maxCeilingResponse = isFallingFast ? 0.5f : 2.0f;
    //     //         float limitedYResponse = response.y < -maxCeilingResponse ? -maxCeilingResponse : response.y;
                
    //     //         playerPosition.y += limitedYResponse;
    //     //         Vector3 vel = m_physics.GetVelocity();
    //     //         vel.y = 0.0f; // Stop upward movement
    //     //         m_physics.SetVelocity(vel);
    //     //     }
    //     // }
    //     // // Wall collision (X or Z axis dominant)
    //     // else
    //     // {
    //     //     // For wall collisions, limit each axis separately
    //     //     Vector3 limitedResponse = response;
    //     //     float maxWallResponse = isFallingFast ? 0.5f : 1.0f;
            
    //     //     if (absX > absZ)
    //     //     {
    //     //         // X-collision
    //     //         if (fabsf(limitedResponse.x) > maxWallResponse)
    //     //         {
    //     //             limitedResponse.x = limitedResponse.x > 0 ? maxWallResponse : -maxWallResponse;
    //     //         }
    //     //         // Reduce X velocity but preserve Y velocity
    //     //         Vector3 currentVel = m_physics.GetVelocity();
    //     //         m_physics.SetVelocity({currentVel.x * 0.3f, currentVel.y, currentVel.z});
    //     //     }
    //     //     else
    //     //     {
    //     //         // Z-collision
    //     //         if (fabsf(limitedResponse.z) > maxWallResponse)
    //     //         {
    //     //             limitedResponse.z = limitedResponse.z > 0 ? maxWallResponse : -maxWallResponse;
    //     //         }
    //     //         // Reduce Z velocity but preserve Y velocity
    //     //         Vector3 currentVel = m_physics.GetVelocity();
    //     //         m_physics.SetVelocity({currentVel.x, currentVel.y, currentVel.z * 0.3f});
    //     //     }
            
    //     //     // Apply limited wall collision response
    //     //     playerPosition = Vector3Add(playerPosition, limitedResponse);
    //     // }
        
    //     // Clamp player to world boundaries
    //     const float WORLD_BOUNDARY = 400.0f;
    //     const float MIN_Y = -50.0f;
    //     const float MAX_Y = 100.0f;
        
    //     if (playerPosition.x > WORLD_BOUNDARY) playerPosition.x = WORLD_BOUNDARY;
    //     else if (playerPosition.x < -WORLD_BOUNDARY) playerPosition.x = -WORLD_BOUNDARY;
        
    //     if (playerPosition.z > WORLD_BOUNDARY) playerPosition.z = WORLD_BOUNDARY;
    //     else if (playerPosition.z < -WORLD_BOUNDARY) playerPosition.z = -WORLD_BOUNDARY;
        
    //     if (playerPosition.y < MIN_Y)
    //     {
    //         playerPosition.y = MIN_Y;
    //         m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
    //         m_physics.SetGroundLevel(true); // Force grounded when hitting bottom boundary
    //     }
    //     else if (playerPosition.y > MAX_Y)
    //     {
    //         playerPosition.y = MAX_Y;
    //         m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
    //     }
        
    //     // Make sure player isn't incorrectly marked as grounded after wall collision
    //     if ((m_isJumping || m_physics.GetVelocityY() < -1.0f) && 
    //         (absX >= absY || absZ >= absY))
    //     {
    //         if (m_physics.IsGrounded() && response.y <= 0.1f)
    //         {
    //             m_physics.SetGroundLevel(false);
    //         }
    //     }
        
    //     SetPlayerPosition(playerPosition);
    //     UpdatePlayerBox();
    // }
    // // else
    // // {
    // //     // No collision detected - check if we should fall
    // //     float groundPlaneTop = PhysicsComponent::GROUND_COLLISION_CENTER.y + 
    // //                           PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;
    // //     float heightAboveGround = playerPosition.y - groundPlaneTop;

    // //     // Fallback ground detection for world floor only
    // //     if (heightAboveGround <= 0.1f && m_physics.GetVelocityY() <= 0.0f)
    // //     {
    // //         // Snap to ground plane top
    // //         m_physics.SetGroundLevel(true);
    // //         m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
    // //         playerPosition.y = groundPlaneTop + 0.05f; // Increased buffer to prevent floating
    // //         SetPlayerPosition(playerPosition);
    // //         TraceLog(LOG_INFO, "Ground plane detection: Player grounded at y=%.2f", playerPosition.y);
    // //     }
    // //     else if (m_physics.IsGrounded() && heightAboveGround > 0.5f)
    // //     {
    // //         // Player was grounded but is now clearly above ground - start falling
    // //         m_physics.SetGroundLevel(false);
    // //         TraceLog(LOG_INFO, "Player ungrounded - height above ground: %.2f", heightAboveGround);
    // //     }
    // // }
    
    // // Final check to ensure player isn't floating
    // if (m_physics.IsGrounded())
    // {
    //     // Make sure velocity is zero when grounded
    //     Vector3 vel = m_physics.GetVelocity();
    //     if (vel.y != 0.0f)
    //     {
    //         m_physics.SetVelocity({vel.x, 0.0f, vel.z});
    //     }
    // }
}

BoundingBox Player::GetPlayerBoundingBox() const // Get bounding box
{
    return m_playerBoundingBox;
}

const PhysicsComponent &Player::GetPhysics() const // Get physics component
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

std::pair<Vector3, Vector3> Player::GetCameraVectors()
{
    const Camera &camera = m_cameraController->GetCamera();

    Vector3 forward = Vector3Subtract(camera.position, camera.target);
    forward.y = 0;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, forward));

    return {forward, right};
}

void Player::ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    // Calculate rotation
    m_rotationY = atan2f(worldMoveDir.x, worldMoveDir.z) * RAD2DEG;

    // Apply movement with speed limiting for collision stability
    Vector3 movement = Vector3Scale(worldMoveDir, m_walkSpeed * deltaTime);

    // Limit movement per frame to prevent tunneling through collision
    float maxMovePerFrame = 0.5f; // Maximum 0.5 units per frame
    float moveLength = Vector3Length(movement);
    if (moveLength > maxMovePerFrame)
    {
        movement = Vector3Scale(Vector3Normalize(movement), maxMovePerFrame);
        TraceLog(LOG_INFO, "🚀 Movement clamped from %.3f to %.3f units", moveLength,
                 maxMovePerFrame);
    }

    // Debug: Log movement when on ground (temporarily disabled to reduce spam)
    // if (Vector3Length(movement) > 0.01f)
    // {
    //     TraceLog(LOG_INFO, "🚶 Ground movement: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f)",
    //              GetPlayerPosition().x, GetPlayerPosition().y, GetPlayerPosition().z,
    //              GetPlayerPosition().x + movement.x, GetPlayerPosition().y + movement.y,
    //              GetPlayerPosition().z + movement.z);
    // }

    // Apply movement with continuous collision detection
    Vector3 currentPos = GetPlayerPosition();
    Vector3 targetPos = Vector3Add(currentPos, movement);

    // Calculate movement distance and direction
    float moveDistance = Vector3Length(movement);

    // Prevent excessive movement that could cause clipping through colliders
    const float MAX_MOVEMENT_PER_FRAME = 5.0f; // Maximum units per frame
    if (moveDistance > MAX_MOVEMENT_PER_FRAME)
    {
        TraceLog(LOG_WARNING, "⚠️ Movement too large (%.2f), clamping to %.2f", moveDistance,
                 MAX_MOVEMENT_PER_FRAME);
        movement = Vector3Scale(Vector3Normalize(movement), MAX_MOVEMENT_PER_FRAME);
        targetPos = Vector3Add(currentPos, movement);
    }

    // Pre-validate movement to prevent getting stuck
    SetPlayerPosition(targetPos);
    UpdatePlayerBox();

    Vector3 response = {};
    if (m_collisionManager.CheckCollision(GetCollision(), response))
    {
        float maxAxisResponse =
            fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        TraceLog(LOG_INFO, "🔍 Movement collision response: max_axis=%.2f, vector=(%.2f,%.2f,%.2f)",
                 maxAxisResponse, response.x, response.y, response.z);

        // Check for special "stuck" marker from CollisionManager
        if (maxAxisResponse > 50.0f)
        { // Check max axis instead of vector length
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR,
                     "🚨 STUCK MARKER DETECTED (max axis: %.2f) - extracting from collider (stuck "
                     "count: %d)",
                     maxAxisResponse, m_stuckCounter);

            // If stuck too many times in a short period, force teleport
            if (m_stuckCounter >= 3)
            {
                TraceLog(LOG_ERROR,
                         "🚨 STUCK TOO MANY TIMES - forcing emergency teleport to spawn");
                SetPlayerPosition(
                    {0.0f, 15.0f, 0.0f}); // Higher spawn to avoid immediate re-collision
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0; // Reset counter
                return;
            }

            if (!ExtractFromCollider())
            {
                // If extraction failed, force teleport to spawn
                TraceLog(LOG_ERROR, "🚨 EXTRACTION FAILED - forcing teleport to spawn");
                SetPlayerPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
            }
            return; // Skip movement this frame
        }

        // Normal collision - try wall sliding
        Vector3 wallNormal = Vector3Normalize(response);
        Vector3 slideDirection = Vector3Subtract(
            movement, Vector3Scale(wallNormal, Vector3DotProduct(movement, wallNormal)));
        Vector3 slidePos = Vector3Add(currentPos, slideDirection);

        // Test if sliding position is safe
        SetPlayerPosition(slidePos);
        UpdatePlayerBox();
        Vector3 slideResponse = {};
        if (!m_collisionManager.CheckCollision(GetCollision(), slideResponse))
        {
            // Sliding is safe
            TraceLog(LOG_INFO, "🚧 Wall sliding successful");
        }
        else
        {
            // Sliding failed, apply collision response
            slidePos = Vector3Add(targetPos, response);
            SetPlayerPosition(slidePos);
            TraceLog(LOG_INFO, "🚧 Wall sliding failed, using collision response");
        }
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
    Vector3 currentPos = GetPlayerPosition();
    UpdatePlayerBox();

    Vector3 response = {};
    if (!m_collisionManager.CheckCollision(GetCollision(), response))
    {
        TraceLog(LOG_INFO, "🔧 No collision detected - player is free");
        return false; // No collision, nothing to extract from
    }

    TraceLog(LOG_WARNING, "🚨 EXTRACTING PLAYER FROM COLLIDER - current pos: (%.2f, %.2f, %.2f)",
             currentPos.x, currentPos.y, currentPos.z);

    // Try more aggressive extraction positions when stuck
    Vector3 safePositions[] = {
        {currentPos.x, currentPos.y + 20.0f, currentPos.z},        // Much higher above
        {currentPos.x, currentPos.y + 15.0f, currentPos.z},        // High above
        {currentPos.x, currentPos.y + 10.0f, currentPos.z},        // Above
        {currentPos.x + 10.0f, currentPos.y + 5.0f, currentPos.z}, // Far right and up
        {currentPos.x - 10.0f, currentPos.y + 5.0f, currentPos.z}, // Far left and up
        {currentPos.x, currentPos.y + 5.0f, currentPos.z + 10.0f}, // Far forward and up
        {currentPos.x, currentPos.y + 5.0f, currentPos.z - 10.0f}, // Far back and up
        {currentPos.x + 5.0f, currentPos.y + 3.0f, currentPos.z},  // Right and up
        {currentPos.x - 5.0f, currentPos.y + 3.0f, currentPos.z},  // Left and up
        {currentPos.x, currentPos.y + 3.0f, currentPos.z + 5.0f},  // Forward and up
        {currentPos.x, currentPos.y + 3.0f, currentPos.z - 5.0f},  // Back and up
        {0.0f, 20.0f, 0.0f},                                       // Very high spawn position
        {0.0f, 10.0f, 0.0f},                                       // High spawn position
        {10.0f, 10.0f, 10.0f},                                     // Far corner position
        {-10.0f, 10.0f, -10.0f},                                   // Far opposite corner
        {5.0f, 5.0f, 5.0f},                                        // Corner position
        {-5.0f, 5.0f, -5.0f},                                      // Opposite corner
        {0.0f, 5.0f, 0.0f},                                        // Elevated spawn position
        {0.0f, 2.0f, 0.0f}                                         // Default spawn position
    };

    for (int i = 0; i < sizeof(safePositions) / sizeof(safePositions[0]); i++)
    {
        Vector3 safePos = safePositions[i];
        SetPlayerPosition(safePos);
        UpdatePlayerBox();
        Vector3 testResponse = {};
        if (!m_collisionManager.CheckCollision(GetCollision(), testResponse))
        {
            TraceLog(LOG_INFO, "🏠 Found safe position [%d]: (%.2f, %.2f, %.2f)", i, safePos.x,
                     safePos.y, safePos.z);
            m_physics.SetVelocity({0.0f, 0.0f, 0.0f}); // Stop all movement
            m_physics.SetGroundLevel(false);           // Reset ground state
            return true;
        }
        else
        {
            TraceLog(LOG_WARNING, "❌ Safe position [%d] failed: (%.2f, %.2f, %.2f)", i, safePos.x,
                     safePos.y, safePos.z);
        }
    }

    // If all safe positions failed, force to spawn position anyway
    SetPlayerPosition({0.0f, 2.0f, 0.0f});
    m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
    m_physics.SetGroundLevel(false);
    TraceLog(LOG_ERROR, "🚨 ALL SAFE POSITIONS FAILED - forced teleport to spawn");
    return true;
}