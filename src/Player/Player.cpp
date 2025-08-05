//
// Created by I#Oleg
//
#include "Player.h"
#include "PositionData.h"
#include "raylib.h"
#include <memory>

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
    Jump();
    UpdateCameraRotation();

    float radius = 5.0f; 
    Vector3 offset = {
        radius * sinf(m_cameraYaw) * cosf(m_cameraPitch),
        radius * sinf(m_cameraPitch) + 2.0f,
        radius * cosf(m_cameraYaw) * cosf(m_cameraPitch)
    };
    Camera& camera = m_cameraController->GetCamera();
    camera.position = Vector3Add(m_playerPosition, offset);
    camera.target = m_playerPosition;

    m_cameraController->Update();
    UpdatePositionHistory();
}

float Player::GetSpeed() { return m_walkSpeed; }

void Player::SetSpeed(const float speed) { this->m_walkSpeed = speed; }

void Player::Move(const Vector3& moveVector)
{
    m_playerPosition = Vector3Add(m_playerPosition, moveVector);
    UpdatePlayerBox();
}

void Player::Jump()
{
    m_physData.m_dt = GetFrameTime();

    if (IsKeyPressed(KEY_SPACE) && m_physData.m_isGrounded)
    {
        m_physData.m_velocityY = m_jumpStrength * 0.8f;
        m_physData.m_isGrounded = false;
        m_isJumping = true;
    }

    if (m_isJumping || !m_physData.m_isGrounded)
    {
        m_physData.m_velocityY -= m_physData.m_gravity * m_physData.m_dt;
        m_playerPosition.y += m_physData.m_velocityY * m_physData.m_dt;
        
        if (m_playerPosition.y <= 1.0f) 
        {
            m_playerPosition.y = 1.0f;
            m_physData.m_velocityY = 0.0f;
            m_physData.m_isGrounded = true;
            m_isJumping = false;
        }
        
        UpdatePlayerBox();
    }
}

void Player::ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget, float jumpOffsetY)
{
    Vector3 desiredTarget = {baseTarget.x, baseTarget.y + jumpOffsetY, baseTarget.z};
    float smoothingSpeed = 8.0f;
    camera.target = Vector3Lerp(camera.target, desiredTarget, smoothingSpeed * GetFrameTime());
    camera.position = Vector3Lerp(camera.position, {camera.position.x, desiredTarget.y, camera.position.z}, smoothingSpeed * GetFrameTime());
}

void Player::UpdatePositionHistory()
{
    m_posData.m_playerVelocity =
        Vector3Subtract(m_posData.m_playerLastPosition, m_posData.m_playerCurrentPosition);
    m_posData.m_playerLastPosition = m_posData.m_playerCurrentPosition;
    m_posData.m_playerCurrentPosition = m_cameraController->GetCamera().position;
}

void Player::ApplyInput()
{
    m_physData.m_dt = GetFrameTime();
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

        finalMove = Vector3Scale(finalMove, m_walkSpeed * m_physData.m_dt);
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

void Player::DrawPlayer()
{
    if (m_useModel && m_playerModel)
    {
        DrawModel(*m_playerModel, m_playerPosition, 1.0f, WHITE);
    }
    else
    {
        DrawCube(m_playerPosition, m_playerSize.x, m_playerSize.y, m_playerSize.z, m_playerColor);
        DrawCubeWires(m_playerPosition, m_playerSize.x, m_playerSize.y, m_playerSize.z, BLACK);
    }
}

void Player::SetPlayerModel(Model* model)
{
    m_playerModel = model;
}

void Player::ToggleModelRendering(bool useModel)
{
    m_useModel = useModel;
}

void Player::UpdateCameraRotation()
{
    Vector2 mouseDelta = GetMouseDelta();
    float sensitivity = 0.005f; 
    m_cameraYaw   -= mouseDelta.x * sensitivity;
    m_cameraPitch -= mouseDelta.y * sensitivity;
    m_cameraPitch = Clamp(m_cameraPitch, -PI/2.0f + 0.1f, PI/2.0f - 0.1f);

}

