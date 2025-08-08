//
// Created by I#Oleg

#include "CameraController.h"
#include "raylib.h"

CameraController::CameraController() : m_camera({0}), m_cameraMode(CAMERA_THIRD_PERSON)
{
    m_camera.position = (Vector3){4.0f, 4.0f, 4.0f}; // Camera position
    m_camera.target = (Vector3){0.0f, 2.0f, 0.0f};   // Camera looking at point
    m_camera.up = (Vector3){0.0f, 1.0f, 0.0f};       // Camera up vector (rotation towards target)
    m_camera.fovy = 90.0f;                           // Camera field-of-view Y
    m_camera.projection = CAMERA_PERSPECTIVE;        // Camera projection typ {
}

Camera &CameraController::GetCamera() { return m_camera; }

int &CameraController::GetCameraMode() { return m_cameraMode; }
void CameraController::SetCameraMode(const int cameraMode) { this->m_cameraMode = cameraMode; }

void CameraController::Update() { UpdateCamera(&m_camera, m_cameraMode); }

void CameraController::UpdateCameraRotation()
{
    Vector2 mouseDelta = GetMouseDelta();
    float sensitivity = 0.005f;
    m_cameraYaw -= mouseDelta.x * sensitivity;
    m_cameraPitch -= mouseDelta.y * sensitivity;
    m_cameraPitch = Clamp(m_cameraPitch, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f);
}

void CameraController::SetFOV(float FOV) {
     this->m_radiusFOV = FOV;
}

void CameraController::ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget,
                                         float jumpOffsetY)
{
    Vector3 desiredTarget = {baseTarget.x, baseTarget.y + jumpOffsetY, baseTarget.z};
    float smoothingSpeed = 8.0f;
    camera.target = Vector3Lerp(camera.target, desiredTarget, smoothingSpeed * GetFrameTime());
    camera.position =
        Vector3Lerp(camera.position, {camera.position.x, desiredTarget.y, camera.position.z},
                    smoothingSpeed * GetFrameTime());
}

float CameraController::GetCameraYaw() const { return m_cameraYaw; }

float CameraController::GetCameraPitch() const { return m_cameraPitch; }

float CameraController::GetCameraSmoothingFactor() const { return m_cameraSmoothingFactor; }

float CameraController::GetFOV() const {
    return m_radiusFOV;
}
