//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : m_playerCurrentPosition(Vector3Zero()), m_playerLastPosition(Vector3Zero()),
                   m_playerVelocity(Vector3Zero()), cameraController(new CameraController) {
}

void Player::Update() {
    ApplyInput();
    Jump();
    cameraController->Update();
    PositionHistory();

}

float Player::GetSpeed() {
    return walkSpeed;
}

void Player::SetSpeed(const float speed) {
    this->walkSpeed = speed;
}

void Player::Move(const Vector3 offset) const {
    cameraController->getCamera().position = Vector3Add(cameraController->getCamera().position, offset);
    cameraController->getCamera().target = Vector3Add(cameraController->getCamera().target, offset);
}

void Player::LoadModelPlayer() {
   // Planning
}

void Player::Jump() {
    dt = GetFrameTime();
    // Set gravity
    if (IsKeyPressed(KEY_SPACE) && m_isGrounded) {
        velocityY = jumpStrength;
        m_isGrounded = false;
    }
    velocityY -= gravity * dt;
    cameraController->getCamera().position.y += velocityY * dt ;
    // Check if player on ground
    if (cameraController->getCamera().position.y <= GroundLevel) {
        cameraController->getCamera().position.y = GroundLevel;
        velocityY = 0;
        m_isGrounded = true;
    }


    cameraController->Update();
}

void Player::PositionHistory() {
    // Needed for player jump
    m_playerVelocity = Vector3Subtract(m_playerLastPosition , m_playerCurrentPosition);
    m_playerLastPosition = m_playerCurrentPosition;
    m_playerCurrentPosition = cameraController->getCamera().position;
}

void Player::ApplyInput() {
    dt = GetFrameTime();
    Vector3 moveDir = {};

    if (IsKeyDown(KEY_W)) moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.z += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;

    walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? runSpeed : 3.1f;

    if (Vector3Length(moveDir) > 0) {
        moveDir = Vector3Normalize(moveDir);

        Vector3 forward = Vector3Subtract(getCameraController()->getCamera().position , getCameraController()->getCamera().target);
        forward.y = 0;
        forward = Vector3Normalize(forward);

        Vector3 right = Vector3CrossProduct((Vector3){ 0, 1, 0 }, forward);
        right = Vector3Normalize(right);

        Vector3 finalMove = {
            right.x * moveDir.x + forward.x * moveDir.z,
            0.0f,
            right.z * moveDir.x + forward.z * moveDir.z
        };

        TraceLog(LOG_INFO, "MoveVec: X: %.2f Y: %.2f Z: %.2f", finalMove.x, finalMove.y, finalMove.z);

        finalMove = Vector3Scale(finalMove, walkSpeed * dt);
        Move(finalMove);
    }
}

CameraController * Player::getCameraController() const {
     return cameraController;
}









