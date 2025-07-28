//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : camera({0}) , cameraMode(CAMERA_FIRST_PERSON) {
    camera.position = { 4.0f, 2.0f, 4.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 60.0f;                                // Camera field-of-view Y
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
    // As default - first person
    UpdateCamera(&camera , cameraMode);
}

float Player::GetSpeed() const {
    return moveSpeed;
}

void Player::SetSpeed(float speed) {
    this->moveSpeed = speed;
}

void Player::Move(const Vector3 offset) {
    camera.position = Vector3Add(camera.position, offset);
    camera.target = Vector3Add(camera.target, offset);
}


