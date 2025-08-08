//
// Created by I#Oleg
//
#include <Player/Player.h>
#include <Player/PositionData.h>
#include <memory>
#include <raylib.h>

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

void Player::Update()
{
    ApplyInput();
    // Jump();
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

// void Player::Jump()
// {
//     m_physData.m_dt = GetFrameTime();

//     if (IsKeyPressed(KEY_SPACE) && m_physData.m_isGrounded)
//     {
//         m_physData.m_velocityY = m_jumpStrength * 0.8f;
//         m_physData.m_isGrounded = false;
//         m_isJumping = true;
//     }

//     if (m_isJumping || !m_physData.m_isGrounded)
//     {
//         m_physData.m_velocityY -= m_physData.m_gravity * m_physData.m_dt;
//         m_playerPosition.y += m_physData.m_velocityY * m_physData.m_dt;

//         if (m_playerPosition.y <= 1.0f)
//         {
//             TraceLog(LOG_WARNING, "Player fell below ground level, resetting position.");
//             m_playerPosition.y = 1.0f;
//             m_physData.m_velocityY = 0.0f;
//             m_physData.m_isGrounded = true;
//             m_isJumping = false;
//         }

//         UpdatePlayerBox();
//     }
// }

void Player::UpdatePositionHistory()
{
    m_posData.m_playerVelocity =
        Vector3Subtract(m_posData.m_playerLastPosition, m_posData.m_playerCurrentPosition);
    m_posData.m_playerLastPosition = m_posData.m_playerCurrentPosition;
    m_posData.m_playerCurrentPosition = m_cameraController->GetCamera().position;
}

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

void Player::UpdateMouseRotation() const {
    // TODO : fix FOV
    float radius = 8.0f; // FOV
    Vector3 offset = {radius * sinf(m_cameraController->GetCameraYaw()) *
                          cosf(m_cameraController->GetCameraPitch()),
                      radius * sinf(m_cameraController->GetCameraPitch()) + 2.0f,
                      radius * cosf(m_cameraController->GetCameraYaw()) *
                          cosf(m_cameraController->GetCameraPitch())};

    if (offset.y < 0.0f)
    {
        offset.y = 0.0f; // Prevent camera from going below ground level
    }
    // Update camera position based on player position and offset
    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        radius += wheel * 2.f; // Adjust radius

        // Clamp radius
        if (radius < 2.0f)
            radius = 2.0f;
        if (radius > 20.0f)
            radius = 20.0f;

        // Recalculate offset based on updated radius and camera angles
        offset = {radius * sinf(m_cameraController->GetCameraYaw()) *
                      cosf(m_cameraController->GetCameraPitch()),
                  radius * sinf(m_cameraController->GetCameraPitch()) + 2.0f,
                  radius * cosf(m_cameraController->GetCameraYaw()) *
                      cosf(m_cameraController->GetCameraPitch())};
    }
    Camera &camera = m_cameraController->GetCamera();
    camera.position = Vector3Add(m_playerPosition, offset);
    camera.target = m_playerPosition;
}

void Player::ApplyGravityForPlayer(const CollisionManager &collisionManager)
{
    float dt = GetFrameTime();

    if (IsKeyDown(KEY_SPACE) && m_physics.IsGrounded())
    {
        m_physics.SetVelocityY(m_jumpStrength );
        m_physics.SetGroundLevel(false);
        m_isJumping = true;
    }

    if (!m_physics.IsGrounded())
    {
        float velocityY = m_physics.GetVelocityY() - m_physics.GetGravity() * dt;
        m_physics.SetVelocityY(velocityY);
        m_playerPosition.y += velocityY * dt;
        UpdatePlayerBox();
    }

    Vector3 response = {};
    bool isColliding = collisionManager.CheckCollision(GetCollision(), response);

    if (isColliding)
    {
        if (response.y > 0.0f)
        {
            m_playerPosition.y += response.y;
            m_physics.SetVelocityY(0.0f);
            m_physics.SetGroundLevel(true);
            m_isJumping = false;
        }
        else if (response.y < 0.0f)
        {
            m_playerPosition.y += response.y;
            m_physics.SetVelocityY(0.0f);
            m_isJumping = false;
        }
        else // Hitting ceiling or side
        {
            m_playerPosition = Vector3Add(m_playerPosition, response);
            m_physics.SetVelocityY(0.0f);

            if (response.y < 0.0f)
            {
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


Matrix Player::GetPlayerRotation() const {
    Vector3 playerPos = m_posData.m_playerCurrentPosition;
    Vector3 cameraTarget = m_cameraController->GetCamera().target;
    Vector3 toCamera = Vector3Subtract(cameraTarget, playerPos);
    float angleY = atan2f(toCamera.x, toCamera.z);
    return MatrixRotateY(angleY);
}

bool Player::IsJumpCollision() const { return m_isJumpCollision; }

Vector3 Player::GetPlayerPosition() const { return m_playerPosition; }
