//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : camera({0}) , cameraMode(CAMERA_FIRST_PERSON) {
    camera.position = { 4.0f, 2.0f, 4.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection typ
}
Camera &Player::getCamera() {
    return camera;
}

int &Player::GetCameraMode() {
    return cameraMode;
}
void Player::SetCameraMode(const int cameraMode) {
    this->cameraMode = cameraMode;
}

void Player::Update() {

    const float dt = GetFrameTime();
    Vector3 moveDir = {};

    if (IsKeyDown(KEY_W)) {
        moveDir.z -= 1.0f;
    }
    if (IsKeyDown(KEY_S)) {
        moveDir.z += 1.0f;
    }
    if (IsKeyDown(KEY_A)) {
        moveDir.x -= 1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        moveDir.x += 1.0f;
    }

    if (Vector3Length(moveDir) > 0) {
        moveDir = Vector3Normalize(moveDir);
        moveDir = Vector3Scale(moveDir, moveSpeed * dt);
        Move(moveDir);
    }
    UpdateCamera(&camera, cameraMode);

    //
    // Take velocity and position for player jump
    //
    m_playerVelocity = Vector3Subtract(m_playerLastPosition , m_playerCurrentPosition);
    m_playerLastPosition = m_playerCurrentPosition;
    m_playerCurrentPosition = camera.position;
    // Set gravity
    camera.position.y -= gravity * dt;

    camera.position.y = Clamp(camera.position.y , 3.5f , 999.9f);

    if (IsKeyPressed(KEY_SPACE)) {
        camera.position.y += jumpStrength;
    }


}

float Player::GetSpeed() const {
    return moveSpeed;
}

void Player::SetSpeed(const float speed) {
    this->moveSpeed = speed;
}

void Player::Move(const Vector3 offset) {
    camera.position = Vector3Add(camera.position, offset);
    camera.target = Vector3Add(camera.target, offset);;
}

void Player::LoadModelPlayer() {
   // Planning
}

void Player::Jump() {
    UpdateCamera(&camera, cameraMode);

    // Take velocity and position
    m_playerVelocity = Vector3Subtract(m_playerLastPosition , m_playerCurrentPosition);
    m_playerLastPosition = m_playerCurrentPosition;
    m_playerCurrentPosition = camera.position;
    // Set gravity
    camera.position.y -= gravity * GetFrameTime();

    camera.position.y = Clamp(camera.position.y , 1.7f , 999.9f);
}









