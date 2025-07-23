//
// Created by I#Oleg on 23.07.2025.
//

#include "Player.h"

Player::Player() {
    camera.position = { 4.0f, 4.0f, 4.0f };
    camera.target = { 0.0f, 1.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

Camera Player::getCamera() const {
    return camera;
}

void Player::Update() {
    UpdateCamera(&camera , CAMERA_FREE);
}


