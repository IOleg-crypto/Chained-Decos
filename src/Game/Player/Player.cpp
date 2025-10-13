#include "PlayerMovement.h"
#include <CameraController/CameraController.h>
#include <Player.h>
#include <World/World.h>
#include <memory>

// Define player constants
const Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 5.0f,
                                                0.0f}; // Safe spawn position above ground
const float Player::MODEL_Y_OFFSET = -1.f;
const float Player::MODEL_SCALE = 1.1f;

Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    TraceLog(LOG_INFO, "Creating Player...");

    // Initialize player bounding box size
    m_boundingBoxSize = (Vector3){1.0f, 2.5f, 1.0f}; // Adjusted height for large model

    // Create component objects
    m_movement = std::make_unique<PlayerMovement>(this);
    m_input = std::make_unique<PlayerInput>(this);
    m_model = std::make_unique<PlayerModel>();
    m_collision = std::make_unique<PlayerCollision>(this);


    // Initialize player position - use safe position above ground
    Vector3 safePosition = {0.0f, 5.0f, 0.0f};
    SetPlayerPosition(safePosition);

    TraceLog(LOG_INFO, "Player::Player() - Player initialized at safe position (%.2f, %.2f, %.2f)",
               safePosition.x, safePosition.y, safePosition.z);

    // Let physics detect ground; start ungrounded so gravity can act
    m_movement->GetPhysics().SetGroundLevel(false);
    m_movement->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Additional safety check
    Vector3 currentPos = GetPlayerPosition();
    TraceLog(LOG_INFO, "Player::Player() - Player current position after init: (%.2f, %.2f, %.2f)",
               currentPos.x, currentPos.y, currentPos.z);

    // Force update collision system
    UpdatePlayerBox();
    UpdatePlayerCollision();

    // Final position verification
    Vector3 finalPos = GetPlayerPosition();
    TraceLog(LOG_INFO, "Player::Player() - Player final position: (%.2f, %.2f, %.2f)",
               finalPos.x, finalPos.y, finalPos.z);

    // Ensure player is properly grounded
    if (finalPos.y < 5.0f)
    {
        Vector3 correctedPos = {finalPos.x, 5.0f, finalPos.z};
        SetPlayerPosition(correctedPos);
        TraceLog(LOG_WARNING, "Player::Player() - Player position corrected to (%.2f, %.2f, %.2f)",
                   correctedPos.x, correctedPos.y, correctedPos.z);
    }

    // Final verification
    Vector3 verifiedPos = GetPlayerPosition();
    TraceLog(LOG_INFO, "Player::Player() - Player verified final position: (%.2f, %.2f, %.2f)",
               verifiedPos.x, verifiedPos.y, verifiedPos.z);

    // Ensure player stays at safe height
    if (verifiedPos.y < 5.0f)
    {
        Vector3 safeCorrectedPos = {verifiedPos.x, 5.0f, verifiedPos.z};
        SetPlayerPosition(safeCorrectedPos);
        TraceLog(LOG_ERROR, "Player::Player() - Player position force-corrected to safe height (%.2f, %.2f, %.2f)",
                   safeCorrectedPos.x, safeCorrectedPos.y, safeCorrectedPos.z);
    }

    // Final safety check - ensure player is at correct height
    Vector3 finalCheckPos = GetPlayerPosition();
    if (finalCheckPos.y < 5.0f)
    {
        Vector3 emergencyPos = {finalCheckPos.x, 5.0f, finalCheckPos.z};
        SetPlayerPosition(emergencyPos);
        TraceLog(LOG_ERROR, "Player::Player() - EMERGENCY: Player position corrected to (%.2f, %.2f, %.2f)",
                   emergencyPos.x, emergencyPos.y, emergencyPos.z);
    }

    // Ultimate safety check
    Vector3 ultimatePos = GetPlayerPosition();
    if (ultimatePos.y < 5.0f)
    {
        Vector3 ultimateSafePos = {ultimatePos.x, 5.0f, ultimatePos.z};
        SetPlayerPosition(ultimateSafePos);
        TraceLog(LOG_ERROR, "Player::Player() - ULTIMATE SAFETY: Player position corrected to (%.2f, %.2f, %.2f)",
                   ultimateSafePos.x, ultimateSafePos.y, ultimateSafePos.z);
    }

    // Final position confirmation
    Vector3 finalConfirmedPos = GetPlayerPosition();
    TraceLog(LOG_INFO, "Player::Player() - Player final confirmed position: (%.2f, %.2f, %.2f)",
               finalConfirmedPos.x, finalConfirmedPos.y, finalConfirmedPos.z);
}

Player::~Player() = default;

// Main update function called every frame
void Player::Update(const CollisionManager &collisionManager)
{
    // Process input first
    m_input->ProcessInput();

    // Update camera
    m_cameraController->UpdateCameraRotation();
    m_cameraController->UpdateMouseRotation(m_cameraController->GetCamera(),
                                            m_movement->GetPosition());
    m_cameraController->Update();

    // Apply physics
    float deltaTime = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);
    m_movement->SetCollisionManager(&collisionManager);

    HandleJumpInput();
    HandleEmergencyReset();

    m_movement->ApplyGravity(deltaTime);

    // Integrate horizontal velocity from physics into desired position
    Vector3 horizVel = m_movement->GetPhysics().GetVelocity();
    horizVel.y = 0.0f;
    if (Vector3Length(horizVel) > 0.0f)
    {
        Vector3 step = Vector3Scale(horizVel, deltaTime);
        m_movement->Move(step);
    }

    Vector3 newPosition = m_movement->StepMovement(collisionManager);

    SetPlayerPosition(newPosition);

    UpdatePlayerBox();
    UpdatePlayerCollision();

    // Only snap when falling slowly to avoid oscillation
    if (!m_movement->GetPhysics().IsGrounded() && m_movement->GetPhysics().GetVelocity().y <= 0.0f)
    {
        m_movement->SnapToGround(collisionManager);
    }

    // Force ground state if player is very close to ground and not moving up fast
    if (!m_movement->GetPhysics().IsGrounded() && m_movement->GetPhysics().GetVelocity().y <= 2.0f)
    {
        Vector3 playerPos = GetPlayerPosition();
        if (playerPos.y <= 1.5f) // More generous ground proximity for jumping
        {
            m_movement->GetPhysics().SetGroundLevel(true);
            m_movement->GetPhysics().SetVelocityY(0.0f);
            TraceLog(LOG_DEBUG, "Player::Update() - Force grounded player at low Y position: %.2f", playerPos.y);
        }
    }

    if (m_movement->GetPhysics().IsGrounded())
    {
        m_isJumping = false;
        // Ensure velocity is zero when grounded to prevent sliding
        Vector3 currentVel = m_movement->GetPhysics().GetVelocity();
        if (fabsf(currentVel.y) < 0.1f)
        {
            currentVel.y = 0.0f;
            m_movement->GetPhysics().SetVelocity(currentVel);
        }
    }
}

float Player::GetSpeed() const { return m_movement->GetSpeed(); }

float Player::GetRotationY() const { return m_movement->GetRotationY(); }

void Player::SetSpeed(const float speed) const { m_movement->SetSpeed(speed); }

void Player::Move(const Vector3 &moveVector) const { m_movement->Move(moveVector); }

// Handle input both on ground and mid-air
void Player::ApplyInput() const { m_input->ProcessInput(); }

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }

ModelLoader &Player::GetModelManager() const { return m_model->GetModelManager(); }

void Player::SetPlayerModel(Model *model) const { m_model->SetModel(model); }

void Player::UpdatePlayerBox() const { m_collision->UpdateBoundingBox(); }

void Player::UpdatePlayerCollision() const { m_collision->Update(); }

void Player::SyncCollision() const {
    UpdatePlayerBox();
    UpdatePlayerCollision();
}

void Player::ToggleModelRendering(const bool useModel) const
{
    m_model->ToggleModelRendering(useModel);
}

void Player::SetPlayerPosition(const Vector3 &pos) const { m_movement->SetPosition(pos); }

const Collision &Player::GetCollision() const { return *m_collision; }

bool Player::IsJumpCollision() const { return m_collision->IsJumpCollision(); }

Vector3 Player::GetPlayerPosition() const { return m_movement->GetPosition(); }

Vector3 Player::GetPlayerSize() const { return m_boundingBoxSize; }

// Apply jump impulse based on mass and direction
void Player::ApplyJumpImpulse(float impulse)
{
    // Delegate jump gating (grounded/coyote-time) to PlayerMovement
    m_movement->ApplyJumpImpulse(impulse);
    if (m_movement->GetPhysics().GetVelocity().y > 0.0f)
        m_isJumping = true;
}

void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    // Legacy function - now delegates to Update()
    Update(collisionManager);
}

void Player::HandleJumpInput() const { m_input->HandleJumpInput(); }

void Player::HandleEmergencyReset() const { m_input->HandleEmergencyReset(); }

void Player::ApplyGravity(const float deltaTime) const { m_movement->ApplyGravity(deltaTime); }

Vector3 Player::StepMovement(const CollisionManager &collisionManager) const
{
    return m_movement->StepMovement(collisionManager);
}

void Player::SnapToGroundIfNeeded(const CollisionManager &collisionManager) const
{
    m_movement->SnapToGround(collisionManager);
}

BoundingBox Player::GetPlayerBoundingBox() const { return m_collision->GetBoundingBox(); }

const PhysicsComponent &Player::GetPhysics() const { return m_movement->GetPhysics(); }

PhysicsComponent &Player::GetPhysics() { return m_movement->GetPhysics(); }

void Player::SetRotationY(const float rotationY) const { m_movement->SetRotationY(rotationY); }

PlayerMovement *Player::GetMovement() const { return m_movement.get(); }
PlayerCollision& Player::GetCollisionMutable() { 
    return *m_collision; 
}

