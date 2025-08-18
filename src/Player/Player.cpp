// Created by I#Oleg
#include <Player/Player.h>
#include <memory>
#include <raylib.h>

// ==================== CONSTANTS DEFINITIONS ====================

// Player constants - spawn above the ground plane center
const Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 5.0f, 0.0f}; // Higher spawn to avoid ground collision issues
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
    // Reset stuck counter after 5 seconds of normal operation
    if (m_stuckCounter > 0 && (GetTime() - m_lastStuckTime) > 5.0f) {
        TraceLog(LOG_INFO, "🔧 Resetting stuck counter after 5 seconds of normal operation");
        m_stuckCounter = 0;
    }
    
    // Reset jumping state after 3 seconds to prevent getting stuck in jumping mode
    if (m_isJumping && (GetTime() - m_jumpStartTime) > 3.0f) {
        TraceLog(LOG_INFO, "🔧 Resetting jumping state after 3 seconds");
        m_isJumping = false;
    }
    
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
        ApplyJumpImpulse(m_physics.GetJumpStrength() * 8.0f); // Reduced jump strength
    }
    
    // Handle manual extraction from collider (R key)
    if (IsKeyPressed(KEY_R))
    {
        TraceLog(LOG_INFO, "🔧 Manual extraction requested");
        ExtractFromCollider();
    }
    
    // Emergency reset to spawn (T key)
    if (IsKeyPressed(KEY_T))
    {
        TraceLog(LOG_WARNING, "🚨 EMERGENCY RESET - teleporting to spawn");
        SetPlayerPosition(DEFAULT_SPAWN_POSITION);
        m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
        m_physics.SetGroundLevel(false);
    }

    // Apply gravity and physics only if not grounded
    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * deltaTime;
        m_physics.SetVelocity(vel);

        // Apply velocity to position with collision checking
        Vector3 move = Vector3Scale(vel, deltaTime);
        Vector3 currentPos = GetPlayerPosition();
        Vector3 newPosition = Vector3Add(currentPos, move);
        
        // For fast movement, check collision along the path
        float moveDistance = Vector3Length(move);
        if (moveDistance > 0.01f) {
            // Break movement into smaller steps for better collision detection
            int steps = (int)(moveDistance / 0.005f) + 1; // 0.5cm steps for maximum precision
            steps = steps > 200 ? 200 : steps; // Limit max steps for performance
            Vector3 stepMove = Vector3Scale(move, 1.0f / steps);
            
            for (int i = 0; i < steps; i++) {
                Vector3 testPos = Vector3Add(currentPos, Vector3Scale(stepMove, i + 1));
                SetPlayerPosition(testPos);
                UpdatePlayerBox();
                
                Vector3 response = {};
                if (collisionManager.CheckCollision(GetCollision(), response)) {
                    // Check for stuck marker first - check max axis instead of vector length
                    float maxAxisResponse = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
                    if (maxAxisResponse > 50.0f) { // Much lower threshold for individual axis
                        m_stuckCounter++;
                        m_lastStuckTime = GetTime();
                        TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED during fall (max axis: %.2f) - extracting (stuck count: %d)", maxAxisResponse, m_stuckCounter);
                        
                        // If stuck too many times, force teleport
                        if (m_stuckCounter >= 3) {
                            TraceLog(LOG_ERROR, "🚨 STUCK TOO MANY TIMES DURING FALL - forcing emergency teleport");
                            SetPlayerPosition(DEFAULT_SPAWN_POSITION);
                            m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                            m_physics.SetGroundLevel(false);
                            m_stuckCounter = 0;
                            return;
                        }
                        
                        // if (!ExtractFromCollider()) {
                        //     // If extraction failed, force teleport to spawn
                        //     TraceLog(LOG_ERROR, "🚨 FALL EXTRACTION FAILED - forcing teleport to spawn");
                        //     SetPlayerPosition({0.0f, 15.0f, 0.0f});
                        //     m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                        //     m_physics.SetGroundLevel(false);
                        // }
                        return; // Skip rest of physics this frame
                    }
                    
                    // Hit something during fall - handle it gently
                    if (response.y > 0.01f) {
                        // Check if this is actually a floor (not a wall) - compare Y to max horizontal
                        float maxHorizontal = fmaxf(fabsf(response.x), fabsf(response.z));
                        bool isFloorDominant = (response.y > maxHorizontal * 1.5f); // Y must be 1.5x larger than horizontal
                        
                        // Only consider it a floor if Y response is clearly dominant
                        if (isFloorDominant && m_physics.GetVelocityY() <= 0.0f) {
                            // Floor collision - land on surface
                            testPos.y += response.y;
                            m_physics.SetGroundLevel(true);
                            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
                            m_isJumping = false;
                            TraceLog(LOG_INFO, "🏠 Landed on floor during fall at step %d: y=%.3f (Y=%.2f vs maxH=%.2f)", i+1, testPos.y, response.y, maxHorizontal);
                        } else {
                            // Wall collision with some Y component - don't ground
                            testPos = Vector3Add(currentPos, Vector3Scale(stepMove, i));
                            TraceLog(LOG_INFO, "🧱 Wall hit during fall at step %d (Y=%.2f vs maxH=%.2f, not grounding)", i+1, response.y, maxHorizontal);
                        }
                    } else {
                        // Wall or ceiling collision - stop at previous safe position
                        testPos = Vector3Add(currentPos, Vector3Scale(stepMove, i));
                        TraceLog(LOG_INFO, "🧱 Wall hit during fall at step %d", i+1);
                    }
                    newPosition = testPos;
                    break;
                }
            }
        }
        
        SetPlayerPosition(newPosition);
        
        // If we handled collision during stepping, skip the main collision check
        if (m_physics.IsGrounded()) {
            return; // Already handled collision during fall
        }
    }

    // Update collision box before checking
    UpdatePlayerBox();

    Vector3 response = {};
    bool isColliding = collisionManager.CheckCollision(GetCollision(), response);

    Vector3 playerPosition = GetPlayerPosition();
    
    if (isColliding)
    {
        // Improved collision response to reduce jittering and fix standing on models
        float maxAxisResponse = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        TraceLog(LOG_INFO, "🔍 Physics collision response: max_axis=%.2f, vector=(%.2f,%.2f,%.2f)", 
                 maxAxisResponse, response.x, response.y, response.z);
        
        // Check for special "stuck" marker from CollisionManager
        if (maxAxisResponse > 50.0f) { // Check max axis instead of vector length
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED in ApplyPhysics (max axis: %.2f) - extracting (stuck count: %d)", maxAxisResponse, m_stuckCounter);
            
            // If stuck too many times, force teleport
            if (m_stuckCounter >= 3) {
                TraceLog(LOG_ERROR, "🚨 STUCK TOO MANY TIMES IN PHYSICS - forcing emergency teleport");
                SetPlayerPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0;
                return;
            }
            
            if (!ExtractFromCollider()) {
                // If extraction failed, force teleport to spawn
                TraceLog(LOG_ERROR, "🚨 PHYSICS EXTRACTION FAILED - forcing teleport to spawn");
                SetPlayerPosition({0.0f, 15.0f, 0.0f});
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
            }
            return; // Skip collision handling this frame
        }
        
        const float MIN_RESPONSE_THRESHOLD = 0.01f; // Increased threshold to reduce micro-collisions
        
        if (maxAxisResponse < MIN_RESPONSE_THRESHOLD) {
            // Ignore micro-collisions that cause jittering
            return;
        }
        
        // Additional check: if player is falling fast, be more gentle with collision response
        Vector3 velocity = m_physics.GetVelocity();
        bool isFallingFast = velocity.y < -10.0f; // Falling faster than 10 units/sec
        
        if (isFallingFast) {
            TraceLog(LOG_INFO, "🚨 Fast fall detected (%.2f), using gentle collision response", velocity.y);
        }

        // Determine collision type based on response direction
        float absX = fabsf(response.x);
        float absY = fabsf(response.y);
        float absZ = fabsf(response.z);

        // Floor/ceiling collision (Y-axis dominant)
        if (absY >= absX && absY >= absZ)
        {
            if (response.y > 0.0f) // Standing on surface (floor collision)
            {
                // Limit floor response to prevent being launched upward
                // Use smaller limit for fast falls to prevent bouncing
                float maxFloorResponse = isFallingFast ? 0.5f : 2.0f;
                float limitedYResponse = response.y > maxFloorResponse ? maxFloorResponse : response.y;
                
                // More strict floor detection - only ground if:
                // 1. Player is falling (not jumping up)
                // 2. Y response is clearly dominant (not just barely)
                // 3. Response is upward (pushing player up from below)
                bool isActualFloor = (m_physics.GetVelocityY() <= 0.0f && !m_isJumping && 
                                     absY > absX * 1.2f && absY > absZ * 1.2f);
                
                if (isActualFloor) {
                    // Move player up to surface
                    playerPosition.y += limitedYResponse;
                    
                    // Set as grounded and stop vertical movement
                    m_physics.SetGroundLevel(true);
                    m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
                    m_isJumping = false;
                    
                    // Add small buffer to prevent immediate re-collision
                    playerPosition.y += 0.001f;
                    
                    TraceLog(LOG_INFO, "🏠 Player grounded on floor (falling velocity: %.2f)", m_physics.GetVelocityY());
                } else {
                    // Not a clear floor - just apply collision response without grounding
                    playerPosition.y += limitedYResponse;
                    TraceLog(LOG_INFO, "🧱 Vertical collision but not grounding (velocity: %.2f, jumping: %s)", 
                             m_physics.GetVelocityY(), m_isJumping ? "yes" : "no");
                }
                
                if (response.y > maxFloorResponse) {
                    TraceLog(LOG_WARNING, "🏠 Floor response limited from %.3f to %.3f", response.y, limitedYResponse);
                }
            }
            else // Hitting ceiling
            {
                // Limit ceiling response to prevent being pushed down too far
                // Use smaller limit for fast movement to prevent violent pushback
                float maxCeilingResponse = isFallingFast ? 0.5f : 2.0f;
                float limitedYResponse = response.y < -maxCeilingResponse ? -maxCeilingResponse : response.y;
                
                playerPosition.y += limitedYResponse;
                Vector3 vel = m_physics.GetVelocity();
                vel.y = 0.0f; // Stop upward movement
                m_physics.SetVelocity(vel);
                
                if (response.y < -maxCeilingResponse) {
                    TraceLog(LOG_WARNING, "🏠 Ceiling response limited from %.3f to %.3f", response.y, limitedYResponse);
                }
            }
        }
        // Wall collision (X or Z axis dominant)
        else
        {
            // For wall collisions, limit each axis separately to preserve direction
            Vector3 limitedResponse = response;
            // Reasonable limits that match CollisionManager clamping
            float maxWallResponseX = isFallingFast ? 0.5f : 1.0f;
            float maxWallResponseZ = isFallingFast ? 0.5f : 1.0f;
            
            if (absX > absZ) {
                // X-collision - limit X response but allow sliding
                if (fabsf(limitedResponse.x) > maxWallResponseX) {
                    limitedResponse.x = limitedResponse.x > 0 ? maxWallResponseX : -maxWallResponseX;
                }
                // Reduce X velocity but preserve Y velocity (allow falling while sliding)
                Vector3 currentVel = m_physics.GetVelocity();
                m_physics.SetVelocity({currentVel.x * 0.3f, currentVel.y, currentVel.z}); // Keep Y unchanged
            } else {
                // Z-collision - limit Z response but allow sliding
                if (fabsf(limitedResponse.z) > maxWallResponseZ) {
                    limitedResponse.z = limitedResponse.z > 0 ? maxWallResponseZ : -maxWallResponseZ;
                }
                // Reduce Z velocity but preserve Y velocity (allow falling while sliding)
                Vector3 currentVel = m_physics.GetVelocity();
                m_physics.SetVelocity({currentVel.x, currentVel.y, currentVel.z * 0.3f}); // Keep Y unchanged
            }
            
            // Apply limited wall collision response
            playerPosition = Vector3Add(playerPosition, limitedResponse);
            
            // Important: Don't set grounded for wall collisions - let player continue falling
            TraceLog(LOG_INFO, "🧱 Wall collision: response(%.3f,%.3f,%.3f) - preserving fall velocity %.2f", 
                     limitedResponse.x, limitedResponse.y, limitedResponse.z, m_physics.GetVelocityY());
        }
        
        // Safety check: prevent teleportation due to extreme collision responses
        Vector3 originalPos = GetPlayerPosition();
        float teleportDistance = Vector3Distance(originalPos, playerPosition);
        // Use smaller teleport limit for fast falls to prevent being thrown off map
        const float MAX_TELEPORT_DISTANCE = isFallingFast ? 1.0f : 3.0f;
        
        if (teleportDistance > MAX_TELEPORT_DISTANCE) {
            TraceLog(LOG_ERROR, "🚨 COLLISION TELEPORT PREVENTED: distance=%.3f, clamping to %.3f", 
                     teleportDistance, MAX_TELEPORT_DISTANCE);
            
            // Clamp the movement to maximum allowed distance
            Vector3 direction = Vector3Subtract(playerPosition, originalPos);
            direction = Vector3Normalize(direction);
            playerPosition = Vector3Add(originalPos, Vector3Scale(direction, MAX_TELEPORT_DISTANCE));
        }
        
        // Additional safety: clamp player to world boundaries
        const float WORLD_BOUNDARY = 400.0f; // Half of ground size (800/2)
        const float MIN_Y = -50.0f; // Minimum Y to prevent falling through world
        const float MAX_Y = 100.0f; // Maximum Y to prevent flying too high
        
        bool clamped = false;
        if (playerPosition.x > WORLD_BOUNDARY) {
            playerPosition.x = WORLD_BOUNDARY;
            clamped = true;
        } else if (playerPosition.x < -WORLD_BOUNDARY) {
            playerPosition.x = -WORLD_BOUNDARY;
            clamped = true;
        }
        
        if (playerPosition.z > WORLD_BOUNDARY) {
            playerPosition.z = WORLD_BOUNDARY;
            clamped = true;
        } else if (playerPosition.z < -WORLD_BOUNDARY) {
            playerPosition.z = -WORLD_BOUNDARY;
            clamped = true;
        }
        
        if (playerPosition.y < MIN_Y) {
            playerPosition.y = MIN_Y;
            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
            clamped = true;
        } else if (playerPosition.y > MAX_Y) {
            playerPosition.y = MAX_Y;
            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
            clamped = true;
        }
        
        if (clamped) {
            TraceLog(LOG_WARNING, "🌍 Player clamped to world boundaries: (%.2f, %.2f, %.2f)", 
                     playerPosition.x, playerPosition.y, playerPosition.z);
        }
        
        // Additional check: If player was jumping/falling and hit a wall (not floor),
        // make sure they're not incorrectly marked as grounded
        if ((m_isJumping || m_physics.GetVelocityY() < -1.0f) && 
            (absX >= absY || absZ >= absY)) {
            // This was primarily a wall collision, not a floor
            // Make sure player isn't grounded unless it was clearly a floor
            if (m_physics.IsGrounded() && response.y <= 0.1f) {
                m_physics.SetGroundLevel(false);
                TraceLog(LOG_INFO, "🚁 Ungrounding player after wall collision (was falling/jumping)");
            }
        }
        
        SetPlayerPosition(playerPosition);
        UpdatePlayerBox();
    }
    else
    {
        // No collision detected - check if we should fall
        // Ground plane top is at Y=0 (center at Y=-5, size 10, so top = -5 + 5 = 0)
        float groundPlaneTop = PhysicsComponent::GROUND_COLLISION_CENTER.y + 
                              PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;
        float heightAboveGround = playerPosition.y - groundPlaneTop;

        // Additional floor detection using downward raycast
        // This helps catch thin floors that might be missed by regular collision
        // Only do this if player is falling (not jumping up)
        if (!m_physics.IsGrounded() && m_physics.GetVelocityY() <= 0.0f && !m_isJumping) {
            // Cast a ray downward from player center
            Vector3 rayStart = playerPosition;
            Vector3 rayDirection = {0.0f, -1.0f, 0.0f};
            
            // Check multiple points around the player for better floor detection
            Vector3 testPoints[] = {
                playerPosition,  // Center
                {playerPosition.x + 0.2f, playerPosition.y, playerPosition.z},      // Right
                {playerPosition.x - 0.2f, playerPosition.y, playerPosition.z},      // Left
                {playerPosition.x, playerPosition.y, playerPosition.z + 0.2f},      // Forward
                {playerPosition.x, playerPosition.y, playerPosition.z - 0.2f}       // Back
            };
            
            for (const Vector3& testPoint : testPoints) {
                // Create a small collision box slightly below the test point
                Vector3 testPos = {testPoint.x, testPoint.y - 0.1f, testPoint.z};
                SetPlayerPosition(testPos);
                UpdatePlayerBox();
                
                Vector3 floorResponse = {};
                if (collisionManager.CheckCollision(GetCollision(), floorResponse)) {
                    if (floorResponse.y > 0.01f) { // Floor detected
                        // Check if this is actually a floor (Y response dominant)
                        float maxHorizontal = fmaxf(fabsf(floorResponse.x), fabsf(floorResponse.z));
                        bool isFloorDominant = (floorResponse.y > maxHorizontal * 1.5f);
                        
                        // Only ground if Y response is clearly dominant
                        if (isFloorDominant) {
                            // Move player to floor surface
                            playerPosition.y = testPos.y + floorResponse.y;
                            m_physics.SetGroundLevel(true);
                            m_physics.SetVelocity({m_physics.GetVelocity().x, 0.0f, m_physics.GetVelocity().z});
                            m_isJumping = false; // Reset jumping state
                            SetPlayerPosition(playerPosition);
                            TraceLog(LOG_INFO, "🏠 Floor detected via raycast: y=%.3f, response.y=%.3f (Y=%.2f vs maxH=%.2f)", 
                                     playerPosition.y, floorResponse.y, floorResponse.y, maxHorizontal);
                        } else {
                            TraceLog(LOG_INFO, "🧱 Wall detected via raycast (Y=%.2f vs maxH=%.2f) - not grounding", floorResponse.y, maxHorizontal);
                            continue; // Try next test point
                        }
                        break;
                    }
                }
            }
            
            // Restore original position if no floor was found
            if (!m_physics.IsGrounded()) {
                SetPlayerPosition(playerPosition);
            }
        }

        // Fallback ground detection for world floor only
        if (heightAboveGround <= 0.1f && heightAboveGround >= -0.1f && m_physics.GetVelocityY() <= 0.0f)
        {
            // Snap to ground plane top
            m_physics.SetGroundLevel(true);
            m_physics.SetVelocity({0, 0, 0});
            playerPosition.y = groundPlaneTop + 0.01f; // Small offset to prevent z-fighting
            SetPlayerPosition(playerPosition);
        }
        else if (m_physics.IsGrounded() && heightAboveGround > 0.5f)
        {
            // Player was grounded but is now clearly above ground - start falling
            m_physics.SetGroundLevel(false);
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
    if (moveLength > maxMovePerFrame) {
        movement = Vector3Scale(Vector3Normalize(movement), maxMovePerFrame);
        TraceLog(LOG_INFO, "🚀 Movement clamped from %.3f to %.3f units", moveLength, maxMovePerFrame);
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
    if (moveDistance > MAX_MOVEMENT_PER_FRAME) {
        TraceLog(LOG_WARNING, "⚠️ Movement too large (%.2f), clamping to %.2f", moveDistance, MAX_MOVEMENT_PER_FRAME);
        movement = Vector3Scale(Vector3Normalize(movement), MAX_MOVEMENT_PER_FRAME);
        targetPos = Vector3Add(currentPos, movement);
    }
    
    // Pre-validate movement to prevent getting stuck
    SetPlayerPosition(targetPos);
    UpdatePlayerBox();
    
    Vector3 response = {};
    if (m_collisionManager.CheckCollision(GetCollision(), response)) {
        float maxAxisResponse = fmaxf(fmaxf(fabsf(response.x), fabsf(response.y)), fabsf(response.z));
        TraceLog(LOG_INFO, "🔍 Movement collision response: max_axis=%.2f, vector=(%.2f,%.2f,%.2f)", 
                 maxAxisResponse, response.x, response.y, response.z);
        
        // Check for special "stuck" marker from CollisionManager
        if (maxAxisResponse > 50.0f) { // Check max axis instead of vector length
            m_stuckCounter++;
            m_lastStuckTime = GetTime();
            TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED (max axis: %.2f) - extracting from collider (stuck count: %d)", maxAxisResponse, m_stuckCounter);
            
            // If stuck too many times in a short period, force teleport
            if (m_stuckCounter >= 3) {
                TraceLog(LOG_ERROR, "🚨 STUCK TOO MANY TIMES - forcing emergency teleport to spawn");
                SetPlayerPosition({0.0f, 15.0f, 0.0f}); // Higher spawn to avoid immediate re-collision
                m_physics.SetVelocity({0.0f, 0.0f, 0.0f});
                m_physics.SetGroundLevel(false);
                m_stuckCounter = 0; // Reset counter
                return;
            }
            
            if (!ExtractFromCollider()) {
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
        Vector3 slideDirection = Vector3Subtract(movement, Vector3Scale(wallNormal, Vector3DotProduct(movement, wallNormal)));
        Vector3 slidePos = Vector3Add(currentPos, slideDirection);
        
        // Test if sliding position is safe
        SetPlayerPosition(slidePos);
        UpdatePlayerBox();
        Vector3 slideResponse = {};
        if (!m_collisionManager.CheckCollision(GetCollision(), slideResponse)) {
            // Sliding is safe
            TraceLog(LOG_INFO, "🚧 Wall sliding successful");
        } else {
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
    if (!m_collisionManager.CheckCollision(GetCollision(), response)) {
        TraceLog(LOG_INFO, "🔧 No collision detected - player is free");
        return false; // No collision, nothing to extract from
    }
    
    TraceLog(LOG_WARNING, "🚨 EXTRACTING PLAYER FROM COLLIDER - current pos: (%.2f, %.2f, %.2f)", 
             currentPos.x, currentPos.y, currentPos.z);
    
    // Try more aggressive extraction positions when stuck
    Vector3 safePositions[] = {
        {currentPos.x, currentPos.y + 20.0f, currentPos.z}, // Much higher above
        {currentPos.x, currentPos.y + 15.0f, currentPos.z}, // High above
        {currentPos.x, currentPos.y + 10.0f, currentPos.z}, // Above
        {currentPos.x + 10.0f, currentPos.y + 5.0f, currentPos.z}, // Far right and up
        {currentPos.x - 10.0f, currentPos.y + 5.0f, currentPos.z}, // Far left and up
        {currentPos.x, currentPos.y + 5.0f, currentPos.z + 10.0f}, // Far forward and up
        {currentPos.x, currentPos.y + 5.0f, currentPos.z - 10.0f}, // Far back and up
        {currentPos.x + 5.0f, currentPos.y + 3.0f, currentPos.z}, // Right and up
        {currentPos.x - 5.0f, currentPos.y + 3.0f, currentPos.z}, // Left and up
        {currentPos.x, currentPos.y + 3.0f, currentPos.z + 5.0f}, // Forward and up
        {currentPos.x, currentPos.y + 3.0f, currentPos.z - 5.0f}, // Back and up
        {0.0f, 20.0f, 0.0f}, // Very high spawn position
        {0.0f, 10.0f, 0.0f}, // High spawn position
        {10.0f, 10.0f, 10.0f}, // Far corner position
        {-10.0f, 10.0f, -10.0f}, // Far opposite corner
        {5.0f, 5.0f, 5.0f}, // Corner position
        {-5.0f, 5.0f, -5.0f}, // Opposite corner
        {0.0f, 5.0f, 0.0f}, // Elevated spawn position
        {0.0f, 2.0f, 0.0f} // Default spawn position
    };
    
    for (int i = 0; i < sizeof(safePositions)/sizeof(safePositions[0]); i++) {
        Vector3 safePos = safePositions[i];
        SetPlayerPosition(safePos);
        UpdatePlayerBox();
        Vector3 testResponse = {};
        if (!m_collisionManager.CheckCollision(GetCollision(), testResponse)) {
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