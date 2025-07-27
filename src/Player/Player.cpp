//
// Created by I#Oleg
//
#include "Player.h"

Player::Player() : camera({0}) {
    camera.position = { 0.0f, 0.0f, 0.0f };
    camera.target = { 0.0f, 1.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}
Camera Player::getCamera() const {
    return camera;
}

void Player::Update() {
    UpdateCamera(&camera , CAMERA_FREE);
}


