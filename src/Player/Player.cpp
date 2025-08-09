// Created by I#Oleg
#include <Player/Player.h>
#include <Player/PositionData.h>
#include <memory>
#include <raylib.h>

// Constructor initializes default player parameters
Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    m_originalCameraTarget = m_cameraController->GetCamera().target;
    m_baseTarget = m_originalCameraTarget;

    m_playerPosition = {0.0f, 1.0f, 0.0f};
    m_playerSize = {1.0f, 2.0f, 1.0f};
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
    UpdateMouseRotation();
    m_cameraController->Update();
    UpdatePositionHistory();
    UpdatePlayerBox();
}

float Player::GetSpeed() { return m_walkSpeed; }
void Player::SetSpeed(const float speed) { this->m_walkSpeed = speed; }

void Player::Move(const Vector3 &moveVector)
{
    m_playerPosition = Vector3Add(m_playerPosition, moveVector);
}

// Store previous and current player position for velocity calculation
void Player::UpdatePositionHistory()
{
    m_posData.m_playerVelocity =
        Vector3Subtract(m_posData.m_playerLastPosition, m_posData.m_playerCurrentPosition);
    m_posData.m_playerLastPosition = m_posData.m_playerCurrentPosition;
    m_posData.m_playerCurrentPosition = m_cameraController->GetCamera().position;
}

// Handle input both on ground and mid-air
void Player::ApplyInput()
{
    m_physics.Update(GetFrameTime());
    Vector3 moveDir = {};

    if (IsKeyDown(KEY_W))
        moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_S))
        moveDir.z += 1.0f;
    if (IsKeyDown(KEY_A))
        moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        moveDir.x += 1.0f;

    m_walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? m_runSpeed : 3.1f;

    // If in air, apply extra velocity instead of teleport movement
    if (!m_physics.IsGrounded())
    {
        if (Vector3Length(moveDir) > 0)
        {
            moveDir = Vector3Normalize(moveDir);

            Vector3 forward = Vector3Subtract(m_cameraController->GetCamera().position,
                                              m_cameraController->GetCamera().target);
            forward.y = 0;
            forward = Vector3Normalize(forward);
            Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, forward));

            Vector3 inputMove = {
                right.x * moveDir.x + forward.x * moveDir.z,
                0.0f,
                right.z * moveDir.x + forward.z * moveDir.z,
            };

            inputMove = Vector3Scale(inputMove, 4.0f * m_physics.GetDeltaTime());
            m_physics.AddVelocity(inputMove);
        }
    }

    // Grounded movement
    if (Vector3Length(moveDir) > 0)
    {
        moveDir = Vector3Normalize(moveDir);

        Vector3 forward = Vector3Subtract(GetCameraController()->GetCamera().position,
                                          GetCameraController()->GetCamera().target);
        forward.y = 0;
        forward = Vector3Normalize(forward);

        Vector3 right = Vector3CrossProduct((Vector3){0, 1, 0}, forward);
        right = Vector3Normalize(right);

        Vector3 finalMove = {right.x * moveDir.x + forward.x * moveDir.z, 0.0f,
                             right.z * moveDir.x + forward.z * moveDir.z};

        finalMove = Vector3Scale(finalMove, m_walkSpeed * m_physics.GetDeltaTime());
        Move(finalMove);
    }
}

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }
Models Player::GetModelManager() { return m_modelPlayer; }
PositionData Player::GetPlayerData() const { return m_posData; }

// Update the bounding box based on player position and size
void Player::UpdatePlayerBox()
{
    m_playerBoundingBox.min = Vector3Subtract(m_playerPosition, Vector3Scale(m_playerSize, 0.5f));
    m_playerBoundingBox.max = Vector3Add(m_playerPosition, Vector3Scale(m_playerSize, 0.5f));
}

void Player::UpdateCollision() { m_collision.Update(m_playerPosition, m_playerSize); }

void Player::SetPlayerModel(Model *model) { m_playerModel = model; }
void Player::ToggleModelRendering(bool useModel) { m_useModel = useModel; }

void Player::SetPlayerPosition(const Vector3 &pos)
{
    m_playerPosition = pos;
    UpdatePlayerBox();
    m_collision.Update(m_playerPosition, m_playerSize);
}

const Collision &Player::GetCollision() const { return m_collision; }

// Update camera offset based on FOV and camera angles
void Player::UpdateMouseRotation() const
{
    float currentFOV = m_cameraController->GetFOV();

    float wheelMove = GetMouseWheelMove();
    currentFOV -= wheelMove * 0.5f; // Adjust sensitivity as needed
    GetCameraController()->SetFOV(currentFOV);
    if (GetCameraController()->GetFOV() < 1.0f)
    {
        GetCameraController()->SetFOV(6.0f);
    }
    if (GetCameraController()->GetFOV() > 40.0f)
    {
        GetCameraController()->SetFOV(40.0f);
    }

    Vector3 offset = {GetCameraController()->GetFOV() * sinf(m_cameraController->GetCameraYaw()) *
                          cosf(m_cameraController->GetCameraPitch()),
                      GetCameraController()->GetFOV() * sinf(m_cameraController->GetCameraPitch()) +
                          2.0f,
                      GetCameraController()->GetFOV() * cosf(m_cameraController->GetCameraYaw()) *
                          cosf(m_cameraController->GetCameraPitch())};

    if (offset.y < 0.0f)
        offset.y = 0.0f;

    Camera &camera = m_cameraController->GetCamera();
    camera.position = Vector3Add(m_playerPosition, offset);
    camera.target = m_playerPosition;
}

// Apply gravity and resolve collisions
void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    m_physics.Update(GetFrameTime());

    if (IsKeyDown(KEY_SPACE) && m_physics.IsGrounded())
    {
        ApplyJumpImpulse(m_physics.GetJumpStrength() * 5.0f);
    }

    if (!m_physics.IsGrounded())
    {
        Vector3 vel = m_physics.GetVelocity();
        vel.y -= m_physics.GetGravity() * m_physics.GetDeltaTime();
        m_physics.SetVelocity(vel);

        Vector3 move = Vector3Scale(vel, m_physics.GetDeltaTime());
        m_playerPosition = Vector3Add(m_playerPosition, move);
        UpdatePlayerBox();
    }

    Vector3 response = {};
    bool isColliding = collisionManager.CheckCollision(GetCollision(), response);

    if (isColliding)
    {
        if (response.y > 0.0f)
        {
            m_playerPosition.y += response.y;
            m_physics.SetVelocity({0, 0, 0});
            m_physics.SetGroundLevel(true);
            m_isJumping = false;
        }
        else if (response.y < 0.0f)
        {
            m_playerPosition.y += response.y;
            m_physics.SetVelocity({0, 0, 0});
            m_isJumping = false;
        }
        else // Hitting ceiling or wall
        {
            m_playerPosition = Vector3Add(m_playerPosition, response);
            m_physics.SetVelocity({0, 0, 0});
            if (response.y < 0.0f)
            {
                m_physics.SetGroundLevel(false);
                m_isJumping = false;
            }
        }

        UpdatePlayerBox();
    }
    else
    {
        m_physics.SetGroundLevel(false);
    }
}

// Compute Y-axis player rotation matrix based on camera
Matrix Player::GetPlayerRotation() const
{
    Vector3 playerPos = m_posData.m_playerCurrentPosition;
    Vector3 cameraTarget = m_cameraController->GetCamera().target;
    Vector3 toCamera = Vector3Subtract(cameraTarget, playerPos);
    float angleY = atan2f(toCamera.x, toCamera.z);
    return MatrixRotateY(angleY);
}

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
