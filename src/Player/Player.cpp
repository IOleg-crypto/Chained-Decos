//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : camera({0}) {
    camera.position = { 4.0f, 4.0f, 4.0f };
    camera.target = { 0.0f, 0.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy =
        90.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}
Camera Player::getCamera() const {
    return camera;
}

void Player::Update() {
    UpdateCamera(&camera , CAMERA_FIRST_PERSON);
}

float Player::GetSpeed() const {
    return moveSpeed;
}

void Player::SetSpeed(float speed) {
    this->moveSpeed = speed;
}

void Player::Move(Vector3 offset) {
    camera.position = Vector3Add(camera.position, offset);
    camera.target = Vector3Add(camera.target, offset);
}


