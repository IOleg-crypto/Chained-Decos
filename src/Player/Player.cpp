//
// Created by I#Oleg
//
#include "Player.h"
#include <memory>

Player::Player() : m_cameraController(std::make_shared<CameraController>()) {

}

Player::~Player() = default;

void Player::Update() {
    ApplyInput();
    Jump();
    m_cameraController->Update();
    UpdatePositionHistory();

}

float Player::GetSpeed() {
    return m_walkSpeed;
}

void Player::SetSpeed(const float speed) {
    this->m_walkSpeed = speed;
}

void Player::Move(const Vector3 offset) const {
    m_cameraController->GetCamera().position = Vector3Add(m_cameraController->GetCamera().position, offset);
    m_cameraController->GetCamera().target = Vector3Add(m_cameraController->GetCamera().target, offset);
}


void Player::Jump() {
    m_physData.m_dt = GetFrameTime();
    // Set gravity
    if (IsKeyPressed(KEY_SPACE) && m_physData.m_isGrounded) {
        m_physData.m_velocityY = m_jumpStrength;
        m_physData.m_isGrounded = false;
    }
    m_physData.m_velocityY -= m_physData.m_gravity * m_physData.m_dt;
    m_cameraController->GetCamera().position.y += m_physData.m_velocityY * m_physData.m_dt ;
    // Check if player on ground
    if (m_cameraController->GetCamera().position.y <= m_physData.m_groundLevel) {
        m_cameraController->GetCamera().position.y = m_physData.m_groundLevel;
        m_physData.m_velocityY = 0;
        m_physData.m_isGrounded = true;
    }

    m_cameraController->Update();
}

void Player::UpdatePositionHistory() {
    // Needed for player jump
    m_posData.m_playerVelocity = Vector3Subtract(m_posData.m_playerLastPosition , m_posData.m_playerCurrentPosition);
    m_posData.m_playerLastPosition = m_posData.m_playerCurrentPosition;
    m_posData.m_playerCurrentPosition = m_cameraController->GetCamera().position;
}

void Player::ApplyInput() {
    m_physData.m_dt = GetFrameTime();
    Vector3 moveDir = {};

    if (IsKeyDown(KEY_W)) moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.z += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;

    m_walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? m_runSpeed : 3.1f;

    if (Vector3Length(moveDir) > 0) {
        moveDir = Vector3Normalize(moveDir);

        Vector3 forward = Vector3Subtract(GetCameraController()->GetCamera().position , GetCameraController()->GetCamera().target);
        forward.y = 0;
        forward = Vector3Normalize(forward);

        Vector3 right = Vector3CrossProduct((Vector3){ 0, 1, 0 }, forward);
        right = Vector3Normalize(right);

        Vector3 finalMove = {
            right.x * moveDir.x + forward.x * moveDir.z,
            0.0f,
            right.z * moveDir.x + forward.z * moveDir.z
        };

        //TraceLog(LOG_INFO, "MoveVec: X: %.2f Y: %.2f Z: %.2f", finalMove.x, finalMove.y, finalMove.z);

        finalMove = Vector3Scale(finalMove, m_walkSpeed * m_physData.m_dt);
        Move(finalMove);
    }
}

std::shared_ptr<CameraController> Player::GetCameraController() const {
     return m_cameraController;
}


Models Player::GetModelManager() {
    return m_modelPlayer;
}

PositionData Player::GetPlayerData() const {
    return m_posData;
}









