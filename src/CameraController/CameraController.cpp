//
// Created by I#Oleg


#include "CameraController.h"

CameraController::CameraController() : camera({0}) , cameraMode(CAMERA_FIRST_PERSON) {
    camera.position = { 4.0f, 2.0f, 4.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection typ {
}

Camera &CameraController::getCamera() {
    return camera;
}

int &CameraController::GetCameraMode() {
    return cameraMode;
}
void CameraController::SetCameraMode(const int cameraMode) {
    this->cameraMode = cameraMode;
}

void CameraController::Update() {
    UpdateCamera(&camera, cameraMode);
}
