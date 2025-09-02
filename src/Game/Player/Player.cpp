#include "PlayerMovement.h"
#include <CameraController/CameraController.h>
#include <Player.h>
#include <World/Physics.h>
#include <memory>

// Define player constants
const Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 3.f,
                                                0.0f}; // Lowered spawn position for large model
const float Player::MODEL_Y_OFFSET = -1.f;
const float Player::MODEL_SCALE = 1.1f;

Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    TraceLog(LOG_INFO, "Creating Player...");

    // Initialize player size
    m_playerSize = (Vector3){1.0f, 2.5f, 1.0f}; // Adjusted height for large model

    // Create component objects
    m_movement = std::make_unique<PlayerMovement>(this);
    m_input = std::make_unique<PlayerInput>(this);
    m_model = std::make_unique<PlayerModel>();
    m_collision = std::make_unique<PlayerCollision>(this);

    // Initialize player position
    SetPlayerPosition(DEFAULT_SPAWN_POSITION);
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
    float deltaTime = GetFrameTime();
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

    if (m_movement->GetPhysics().IsGrounded())
    {
        m_isJumping = false;
    }
}

float Player::GetSpeed() { return m_movement->GetSpeed(); }

float Player::GetRotationY() const { return m_movement->GetRotationY(); }

void Player::SetSpeed(const float speed) const { m_movement->SetSpeed(speed); }

void Player::Move(const Vector3 &moveVector) const { m_movement->Move(moveVector); }

// Handle input both on ground and mid-air
void Player::ApplyInput() const { m_input->ProcessInput(); }

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }

Models &Player::GetModelManager() const { return m_model->GetModelManager(); }

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

const Collision &Player::GetCollision() const { return m_collision->GetCollision(); }

bool Player::IsJumpCollision() const { return m_collision->IsJumpCollision(); }

Vector3 Player::GetPlayerPosition() const { return m_movement->GetPosition(); }

Vector3 Player::GetPlayerSize() const { return m_playerSize; }

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