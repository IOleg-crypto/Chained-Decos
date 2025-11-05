//
// Created by I#Oleg

#include "CameraController.h"
#include "raylib.h"
#include <imgui/imgui.h>
#include "rlgl.h"
#include <cmath>

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

// Static utility function for filtering mouse delta
Vector2 CameraController::FilterMouseDelta(const Vector2& mouseDelta)
{
    // Constants for filtering
    const float maxDelta = 200.0f;
    const float glitchThreshold = 1000.0f; // Increased to catch glitches like 96074, 213165
    const float MOUSE_DEAD_ZONE = 0.1f;
    
    Vector2 filtered = mouseDelta;
    
    // Step 1: Completely ignore extreme glitches (like 96074, 213165)
    if (fabs(filtered.x) > glitchThreshold || fabs(filtered.y) > glitchThreshold)
    {
        static int glitchCounter = 0;
        if (glitchCounter++ % 300 == 0) // Log every 5 seconds at 60 FPS
        {
            TraceLog(LOG_WARNING, "CameraController: Mouse delta glitch detected (%.2f, %.2f) - ignored",
                     mouseDelta.x, mouseDelta.y);
        }
        return {0.0f, 0.0f};
    }
    
    // Step 2: Clamp reasonable but large values using raylib Clamp
    filtered.x = Clamp(filtered.x, -maxDelta, maxDelta);
    filtered.y = Clamp(filtered.y, -maxDelta, maxDelta);
    
    // Step 3: Dead zone - ignore very small movements
    if (Vector2Length(filtered) < MOUSE_DEAD_ZONE)
    {
        filtered = {0.0f, 0.0f};
    }
    
    return filtered;
}

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

    //UpdateCamera(&m_camera, m_cameraMode);
}

void CameraController::UpdateCameraRotation()
{
    // Don't update if ImGui captured the mouse
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    Vector2 mouseDelta;
    
    // Alternative approach for Linux/VM - use GetMousePosition() instead of GetMouseDelta()
    // This is more reliable on VMs where GetMouseDelta() can return glitches like 96074, 213165
    static Vector2 lastMousePos = {-1.0f, -1.0f};
    Vector2 currentMousePos = GetMousePosition();
    
    if (lastMousePos.x < 0.0f || lastMousePos.y < 0.0f)
    {
        // First frame - initialize
        lastMousePos = currentMousePos;
        return;
    }
    
    // Calculate delta manually
    mouseDelta.x = currentMousePos.x - lastMousePos.x;
    mouseDelta.y = currentMousePos.y - lastMousePos.y;
    
    // Additional check: if position jump is too large, it's likely a glitch
    // This handles cases where GetMousePosition() itself returns glitches
    const float maxPositionJump = 500.0f;
    if (fabs(mouseDelta.x) > maxPositionJump || fabs(mouseDelta.y) > maxPositionJump)
    {
        // Position glitch detected - ignore this frame and reset lastMousePos
        static int jumpCounter = 0;
        if (jumpCounter++ % 300 == 0)
        {
            TraceLog(LOG_WARNING, "CameraController: Mouse position jump detected (%.2f, %.2f) - resetting",
                     mouseDelta.x, mouseDelta.y);
        }
        lastMousePos = currentMousePos;
        return;
    }
    
    lastMousePos = currentMousePos;
    
    // Diagnostic logging (can be removed later)
    static int logCounter = 0;
    if (logCounter++ % 180 == 0) // Log every 3 seconds at 60 FPS
    {
        TraceLog(LOG_DEBUG, "CameraController: Manual mouseDelta=(%.2f, %.2f)", 
                 mouseDelta.x, mouseDelta.y);
    }
    
    // Apply centralized filtering to prevent glitches
    Vector2 filteredBefore = mouseDelta;
    mouseDelta = FilterMouseDelta(mouseDelta);
    
    if (logCounter % 180 == 0 && (filteredBefore.x != mouseDelta.x || filteredBefore.y != mouseDelta.y))
    {
        TraceLog(LOG_DEBUG, "CameraController: Filtered mouseDelta=(%.2f, %.2f) from (%.2f, %.2f)",
                 mouseDelta.x, mouseDelta.y, filteredBefore.x, filteredBefore.y);
    }

    // Smooth movement smoothing using correct lerp coefficient
    const float smoothingFactor = 0.3f; // 0.3 = smooth smoothing
    m_smoothedMouseDelta = Vector2Lerp(m_smoothedMouseDelta, mouseDelta, smoothingFactor);

    // Actual effect on yaw/pitch
    const float sensitivity = 0.1f; // Standard sensitivity
    m_cameraYaw   -= m_smoothedMouseDelta.x * sensitivity;
    m_cameraPitch -= m_smoothedMouseDelta.y * sensitivity;

    // Angle clamping using raylib Clamp
    m_cameraPitch = Clamp(m_cameraPitch, -PI/2.0f + 0.1f, PI/2.0f - 0.1f);
    
    if (logCounter % 180 == 0)
    {
        TraceLog(LOG_DEBUG, "CameraController: yaw=%.4f, pitch=%.4f, smoothedDelta=(%.2f, %.2f)",
                 m_cameraYaw, m_cameraPitch, m_smoothedMouseDelta.x, m_smoothedMouseDelta.y);
    }
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