//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : cameraController(new CameraController) {
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
    physData.dt = GetFrameTime();
    // Set gravity
    if (IsKeyPressed(KEY_SPACE) && physData.m_isGrounded) {
        physData.velocityY = jumpStrength;
        physData.m_isGrounded = false;
    }
    physData.velocityY -= physData.gravity * physData.dt;
    cameraController->getCamera().position.y += physData.velocityY * physData.dt ;
    // Check if player on ground
    if (cameraController->getCamera().position.y <= physData.GroundLevel) {
        cameraController->getCamera().position.y = physData.GroundLevel;
        physData.velocityY = 0;
        physData.m_isGrounded = true;
    }


    cameraController->Update();
}

void Player::PositionHistory() {
    // Needed for player jump
    posData.m_playerVelocity = Vector3Subtract(posData.m_playerLastPosition , posData.m_playerCurrentPosition);
    posData.m_playerLastPosition = posData.m_playerCurrentPosition;
    posData.m_playerCurrentPosition = cameraController->getCamera().position;
}

void Player::ApplyInput() {
    physData.dt = GetFrameTime();
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

        finalMove = Vector3Scale(finalMove, walkSpeed * physData.dt);
        Move(finalMove);
    }
}

CameraController * Player::getCameraController() const {
     return cameraController;
}









