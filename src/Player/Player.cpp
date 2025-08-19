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

    // Initialize player size
    m_playerSize = {1.0f, 3.5f, 1.0f};
    m_playerColor = BLUE;
    
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
void Player::Update()
{
    ApplyInput();
    m_cameraController->UpdateCameraRotation();
    
    // Тепер метод приймає константне посилання, тому можемо передати результат GetPosition() напряму
    m_cameraController->UpdateMouseRotation(m_cameraController->GetCamera(), m_movement->GetPosition());
    
    m_cameraController->Update();
    UpdatePlayerBox();
}

float Player::GetSpeed()
{
    return m_movement->GetSpeed();
}

float Player::GetRotationY() const
{
    return m_movement->GetRotationY();
}

void Player::SetSpeed(const float speed)
{
    m_movement->SetSpeed(speed);
}

void Player::Move(const Vector3 &moveVector)
{
    m_movement->Move(moveVector);
}

// Handle input both on ground and mid-air
void Player::ApplyInput()
{
    m_input->ProcessInput();
}

std::shared_ptr<CameraController> Player::GetCameraController() const
{
    return m_cameraController;
}

Models &Player::GetModelManager()
{
    return m_model->GetModelManager();
}

// Update the bounding box based on player position and size
void Player::UpdatePlayerBox()
{
    m_collision->UpdateBoundingBox();
}

void Player::UpdatePlayerCollision()
{
    m_collision->Update();
}

void Player::SetPlayerModel(Model *model)
{
    m_model->SetModel(model);
}

void Player::ToggleModelRendering(bool useModel)
{
    m_model->ToggleModelRendering(useModel);
}

void Player::SetPlayerPosition(const Vector3 &pos)
{
    m_movement->SetPosition(pos);
    UpdatePlayerBox();
    UpdatePlayerCollision();
}

const Collision &Player::GetCollision() const
{
    return m_collision->GetCollision();
}

bool Player::IsJumpCollision() const
{
    return m_collision->IsJumpCollision();
}

Vector3 Player::GetPlayerPosition() const
{
    return m_movement->GetPosition();
}

Vector3 Player::GetPlayerSize() const
{
    return m_playerSize;
}

// Apply jump impulse based on mass and direction
void Player::ApplyJumpImpulse(float impulse)
{
    if (!m_movement->GetPhysics().IsGrounded())
        return;

    m_movement->ApplyJumpImpulse(impulse);
    m_isJumping = true;
    m_jumpStartTime = GetTime();
}

void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    float deltaTime = GetFrameTime();

    m_movement->SetCollisionManager(&collisionManager);
    
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
    m_input->HandleJumpInput();
}

void Player::HandleEmergencyReset()
{
    m_input->HandleEmergencyReset();
}

void Player::ApplyGravity(float deltaTime)
{
    m_movement->ApplyGravity(deltaTime);
}

Vector3 Player::StepMovement(const CollisionManager &collisionManager)
{
    return m_movement->StepMovement(collisionManager);
}

void Player::ResolveCollision(const Vector3 &response)
{
    m_movement->ResolveCollision(response);
}

void Player::SnapToGroundIfNeeded(const CollisionManager &collisionManager)
{
    m_movement->SnapToGroundIfNeeded(collisionManager);
}

BoundingBox Player::GetPlayerBoundingBox() const
{
    return m_collision->GetBoundingBox();
}

const PhysicsComponent &Player::GetPhysics() const
{
    return m_movement->GetPhysics();
}

PhysicsComponent &Player::GetPhysics()
{
    return m_movement->GetPhysics();
}

void Player::ApplyGroundedMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    m_movement->ApplyGroundedMovement(worldMoveDir, deltaTime);
}

void Player::ApplyAirborneMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    m_movement->ApplyAirborneMovement(worldMoveDir, deltaTime);
}

void Player::WallSlide(const Vector3 &currentPos, const Vector3 &movement, const Vector3 &response)
{
    m_movement->WallSlide(currentPos, movement, response);
}

bool Player::TryStepUp(const Vector3 &targetPos, const Vector3 &response)
{
    return m_movement->TryStepUp(targetPos, response);
}

Vector3 Player::ClampMovementPerFrame(const Vector3 &movement, float maxMove)
{
    return m_movement->ClampMovementPerFrame(movement, maxMove);
}