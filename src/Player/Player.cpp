// Created by I#Oleg
#include <Player/Player.h>
#include <memory>
#include <raylib.h>

// ==================== CONSTANTS DEFINITIONS ====================

// Player constants
const Vector3 Player::DEFAULT_SPAWN_POSITION = {90.0f, 50.0f, 150.0f};
const float Player::MODEL_Y_OFFSET = -1.2f;
const float Player::MODEL_SCALE = 1.0f;

// Constructor initializes default player parameters
Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    m_originalCameraTarget = m_cameraController->GetCamera().target;
    m_baseTarget = m_originalCameraTarget;

    m_playerPosition = {0.0f, 10.0f, 0.0f};
    // Depends on model size(bounding box collision)
    m_playerSize = {1.0f, 5.5f, 1.0f};
    m_playerColor = BLUE;
    m_playerModel = nullptr;
    m_useModel = false;
    UpdatePlayerBox();
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

    Vector3 jumpVelocity = {0, m_physics.GetJumpStrength(), 0};

    // Estimate mass from player size
    float mass = m_playerSize.x * m_playerSize.y * m_playerSize.z * 1.0f;
    if (mass <= 0.0f)
        mass = 1.0f;

    float verticalVelocity = impulse / mass;
    Vector3 forwardImpulse = {};

    Vector3 forward = Vector3Subtract(m_cameraController->GetCamera().position,
                                      m_cameraController->GetCamera().target);
    forward.y = 0;
    forward = Vector3Normalize(forward);

    Vector3 horizontalVelocity = Vector3Scale(forward, m_walkSpeed);
    jumpVelocity.x = horizontalVelocity.x;
    jumpVelocity.z = horizontalVelocity.z;

    m_physics.SetVelocity({forwardImpulse.x, verticalVelocity, forwardImpulse.z});
    m_physics.SetGroundLevel(false);
    m_isJumping = true;
}

void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    // Handle jumping
    if (IsKeyDown(KEY_SPACE) && m_physics.IsGrounded())
    {
        ApplyJumpImpulse(m_physics.GetJumpStrength() * 5.0f);
    }

    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * GetFrameTime();
        m_physics.SetVelocity(vel);

        Vector3 move = Vector3Scale(vel, m_physics.GetDeltaTime());
        Vector3 currentPlayerPosition = Vector3Add(GetPlayerPosition(), move);
        SetPlayerPosition(currentPlayerPosition);
        UpdatePlayerBox();
    }

    // ALWAYS update player box before collision check to ensure accurate collision detection
    UpdatePlayerBox();

    Vector3 response = {};
    bool isColliding = collisionManager.CheckCollision(GetCollision(), response);

    Vector3 playerPosition = GetPlayerPosition();
    if (isColliding)
    {
        if (response.y > 0.0f)
        {
            playerPosition.y += response.y;
            m_physics.SetVelocity({0, 0, 0});
            m_physics.SetGroundLevel(true);
            m_isJumping = false;
        }
        else if (response.y < 0.0f)
        {
            playerPosition.y += response.y;
            m_physics.SetVelocity({0, 0, 0});
            m_isJumping = false;
        }
        else // Hitting ceiling or wall
        {
            playerPosition = Vector3Add(playerPosition, response);
            m_physics.SetVelocity({0, 0, 0});
            if (response.y < 0.0f)
            {
                m_physics.SetGroundLevel(false);
                m_isJumping = false;
            }
        }
        SetPlayerPosition(playerPosition);
        UpdatePlayerBox();
    }
    else
    {
        m_physics.SetGroundLevel(false);
    }
}

BoundingBox Player::GetPlayerBoundingBox() const // Get bounding box
{
    return m_playerBoundingBox;
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

    // Apply movement
    Vector3 movement = Vector3Scale(worldMoveDir, m_walkSpeed * deltaTime);

    // Debug: Log movement when on ground (temporarily disabled to reduce spam)
    // if (Vector3Length(movement) > 0.01f)
    // {
    //     TraceLog(LOG_INFO, "🚶 Ground movement: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f)",
    //              GetPlayerPosition().x, GetPlayerPosition().y, GetPlayerPosition().z,
    //              GetPlayerPosition().x + movement.x, GetPlayerPosition().y + movement.y,
    //              GetPlayerPosition().z + movement.z);
    // }

    Move(movement);
}

void Player::ApplyAirborneMovement(const Vector3 &worldMoveDir, float deltaTime)
{
    // In air: apply velocity instead of direct movement
    Vector3 airMovement = Vector3Scale(worldMoveDir, 20.0f * deltaTime);
    m_physics.AddVelocity(airMovement);
}