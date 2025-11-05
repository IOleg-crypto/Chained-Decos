//
// Created by I#Oleg

#include "CameraController.h"
#include "raylib.h"
#include <imgui/imgui.h>

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

void CameraController::Update()
{
    // Skip camera update if no window is available (for testing)
    if (!IsWindowReady())
    {
        return;
    }

    // Skip camera update if ImGui wants to capture mouse (menu is open)
    // This prevents UpdateCamera from centering the cursor
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    UpdateCamera(&m_camera, m_cameraMode);
}

void CameraController::UpdateCameraRotation()
{
    // Skip mouse input if no window is available (for testing)
    if (!IsWindowReady())
    {
        return;
    }

    // Skip camera rotation if ImGui wants to capture mouse (menu is open)
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }

    Vector2 mouseDelta = GetMouseDelta();
    float sensitivity = 0.005f;
    m_cameraYaw -= mouseDelta.x * sensitivity;
    m_cameraPitch -= mouseDelta.y * sensitivity;
    m_cameraPitch = Clamp(m_cameraPitch, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f);
}

void CameraController::SetFOV(float FOV) { this->m_radiusFOV = FOV; }

void CameraController::ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget,
                                          float jumpOffsetY)
{
    Vector3 desiredTarget = {baseTarget.x, baseTarget.y + jumpOffsetY, baseTarget.z};
    float smoothingSpeed = 8.0f;

    // Use a default delta time if no window is available (for testing)
    float deltaTime = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);

    camera.target = Vector3Lerp(camera.target, desiredTarget, smoothingSpeed * deltaTime);
    camera.position =
        Vector3Lerp(camera.position, {camera.position.x, desiredTarget.y, camera.position.z},
                    smoothingSpeed * deltaTime);
}

float CameraController::GetCameraYaw() const { return m_cameraYaw; }

float CameraController::GetCameraPitch() const { return m_cameraPitch; }

float CameraController::GetCameraSmoothingFactor() const { return m_cameraSmoothingFactor; }

float CameraController::GetFOV() const { return m_radiusFOV; }

void CameraController::UpdateMouseRotation(Camera &camera, const Vector3 &playerPosition)
{
    // Skip mouse input if no window is available (for testing)
    if (!IsWindowReady())
    {
        // Set default camera position for testing
        Vector3 offset = {GetFOV() * sinf(GetCameraYaw()) * cosf(GetCameraPitch()),
                          GetFOV() * sinf(GetCameraPitch()) + 5.0f,
                          GetFOV() * cosf(GetCameraYaw()) * cosf(GetCameraPitch())};
        camera.position = Vector3Add(playerPosition, offset);
        camera.target = playerPosition;
        return;
    }

    float currentFOV = GetFOV();
    float wheelMove = GetMouseWheelMove();
    currentFOV -= wheelMove * 0.5f; // Adjust sensitivity as needed
    SetFOV(currentFOV);
    if (GetFOV() < 1.0f)
    {
        SetFOV(6.0f);
    }
    if (GetFOV() > 40.0f)
    {
        SetFOV(40.0f);
    }

    Vector3 offset = {GetFOV() * sinf(GetCameraYaw()) * cosf(GetCameraPitch()),
                      GetFOV() * sinf(GetCameraPitch()) + 5.0f,
                      GetFOV() * cosf(GetCameraYaw()) * cosf(GetCameraPitch())};

    // if (offset.y < 0.0f)
    //     offset.y = 0.0f;

    camera.position = Vector3Add(playerPosition, offset);
    camera.target = playerPosition;
}
