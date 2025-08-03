//
// Created by I#Oleg


#include "CameraController.h"

CameraController::CameraController() : m_camera({0}) , m_cameraMode(CAMERA_FIRST_PERSON) {
    m_camera.position = { 4.0f, 2.0f, 4.0f }; // Camera position
    m_camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    m_camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    m_camera.fovy = 90.0f;                                // Camera field-of-view Y
    m_camera.projection = CAMERA_PERSPECTIVE;             // Camera projection typ {
}

Camera &CameraController::GetCamera() {
    return m_camera;
}

int &CameraController::GetCameraMode() {
    return m_cameraMode;
}
void CameraController::SetCameraMode(const int cameraMode) {
    this->m_cameraMode = cameraMode;
}

void CameraController::Update() {
    UpdateCamera(&m_camera, m_cameraMode);
}
