#include "core/log.h"
//
// Created by I#Oleg

#include "camera_controller.h"
#include "core/log.h"

#include "events/event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include <cmath>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

CameraController::CameraController() : m_camera({0}), m_cameraMode(CAMERA_FREE)
{
    m_camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    m_camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    m_camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    m_camera.fovy = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;

    // Calculate initial yaw/pitch from position -> target
    Vector3 dir = Vector3Normalize(Vector3Subtract(m_camera.target, m_camera.position));
    m_cameraPitch = asinf(dir.y) * RAD2DEG;
    m_cameraYaw = atan2f(dir.x, dir.z) * RAD2DEG;
}

Camera &CameraController::GetCamera()
{
    return m_camera;
}

int &CameraController::GetCameraMode()
{
    return m_cameraMode;
}
void CameraController::SetCameraMode(const int cameraMode)
{
    this->m_cameraMode = cameraMode;
}

// Static utility function for filtering mouse delta
Vector2 CameraController::FilterMouseDelta(const Vector2 &mouseDelta)
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
            CD_CORE_WARN("CameraController: Mouse delta glitch detected (%.2f, %.2f) - ignored",
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
        return;

    const ImGuiIO &io = ImGui::GetIO();
    float deltaTime = GetFrameTime();

    // If we are NOT in bypass mode (not in viewport), reset states
    if (!m_inputCaptureBypass)
    {
        m_isLMBDown = false;
        m_isRMBDown = false;
        m_activeMovementKeys = 0;
        return;
    }

    // Update state using ImGui polling for reliability
    m_isLMBDown = ImGui::IsMouseDown(0);
    m_isRMBDown = ImGui::IsMouseDown(1);

    // Update screen shake
    UpdateScreenShake(deltaTime);

    // Handle Cursor state for Fly Mode (RMB)
    if (m_cameraMode == CAMERA_FREE && m_isRMBDown)
    {
        if (!IsCursorHidden())
            DisableCursor();
    }
    else
    {
        if (IsCursorHidden())
            EnableCursor();
    }

    if (m_cameraMode == CAMERA_FREE)
    {
        // 1. Rotation (Mouse) - ONLY with RMB
        if (m_isRMBDown)
        {
            Vector2 mouseDelta = {io.MouseDelta.x, io.MouseDelta.y};

            // Filter huge spikes
            if (Vector2Length(mouseDelta) > 100.0f)
                mouseDelta = {0, 0};

            m_cameraYaw -= mouseDelta.x * m_mouseSensitivity * 0.5f;
            m_cameraPitch -= mouseDelta.y * m_mouseSensitivity * 0.5f;
            m_cameraPitch = Clamp(m_cameraPitch, -89.0f, 89.0f);
        }

        // Calculate direction vectors (needed for movement)
        Vector3 forward = {sinf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD),
                           sinf(m_cameraPitch * DEG2RAD),
                           cosf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD)};
        Vector3 right = {sinf((m_cameraYaw - 90.0f) * DEG2RAD), 0,
                         cosf((m_cameraYaw - 90.0f) * DEG2RAD)};

        // 2. Movement (Keyboard) - WORKS WITHOUT RMB
        float speed = 5.0f * deltaTime;
        if (IsKeyDown(KEY_LEFT_SHIFT))
            speed *= 3.0f;

        bool moved = false;
        if (IsKeyDown(KEY_W))
        {
            m_camera.position = Vector3Add(m_camera.position, Vector3Scale(forward, speed));
            moved = true;
        }
        if (IsKeyDown(KEY_S))
        {
            m_camera.position = Vector3Subtract(m_camera.position, Vector3Scale(forward, speed));
            moved = true;
        }
        if (IsKeyDown(KEY_A))
        {
            m_camera.position = Vector3Subtract(m_camera.position, Vector3Scale(right, speed));
            moved = true;
        }
        if (IsKeyDown(KEY_D))
        {
            m_camera.position = Vector3Add(m_camera.position, Vector3Scale(right, speed));
            moved = true;
        }
        if (IsKeyDown(KEY_E))
        {
            m_camera.position.y += speed;
            moved = true;
        }
        if (IsKeyDown(KEY_Q))
        {
            m_camera.position.y -= speed;
            moved = true;
        }

        if (moved || m_isRMBDown)
            m_camera.target = Vector3Add(m_camera.position, forward);
    }
    else if (m_lastMouseWheelMove != 0.0f || (m_isRMBDown && m_cameraMode != CAMERA_FREE))
    {
        // Standard raylib camera update for other modes or zoom
        UpdateCamera(&m_camera, m_cameraMode);
    }

    m_lastMouseWheelMove = 0.0f;
}

void CameraController::UpdateCameraRotation()
{
    // Don't update if ImGui captured the mouse
    const ImGuiIO &io = ImGui::GetIO();
    if (!m_inputCaptureBypass && io.WantCaptureMouse)
        return;

    Vector2 mouseDelta;

    // Alternative approach for Linux/VM - use GetMousePosition() instead of GetMouseDelta()
    // This is more reliable on VMs where GetMouseDelta() can return glitches like 96074, 213165
    // Use ImGui mouse position for reliability
    static Vector2 lastMousePos = {-1.0f, -1.0f};
    ImVec2 imMouse = ImGui::GetMousePos();
    Vector2 currentMousePos = {imMouse.x, imMouse.y};

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
            CD_CORE_WARN("CameraController: Mouse position jump detected (%.2f, %.2f) - resetting",
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
        CD_CORE_TRACE("CameraController: Manual mouseDelta=(%.2f, %.2f)", mouseDelta.x,
                      mouseDelta.y);
    }

    // Apply centralized filtering to prevent glitches
    Vector2 filteredBefore = mouseDelta;
    mouseDelta = FilterMouseDelta(mouseDelta);

    if (logCounter % 180 == 0 &&
        (filteredBefore.x != mouseDelta.x || filteredBefore.y != mouseDelta.y))
    {
        CD_CORE_TRACE("CameraController: Filtered mouseDelta=(%.2f, %.2f) from (%.2f, %.2f)",
                      mouseDelta.x, mouseDelta.y, filteredBefore.x, filteredBefore.y);
    }

    // Smooth movement smoothing using correct lerp coefficient
    const float smoothingFactor = 0.3f; // 0.3 = smooth smoothing
    m_smoothedMouseDelta = Vector2Lerp(m_smoothedMouseDelta, mouseDelta, smoothingFactor);

    // Actual effect on yaw/pitch
    m_cameraYaw -= m_smoothedMouseDelta.x * m_mouseSensitivity;
    m_cameraPitch -= m_smoothedMouseDelta.y * m_mouseSensitivity;

    // Angle clamping using degrees
    m_cameraPitch = Clamp(m_cameraPitch, -89.0f, 89.0f);

    if (logCounter % 180 == 0)
    {
        CD_CORE_TRACE("CameraController: yaw=%.4f, pitch=%.4f, smoothedDelta=(%.2f, %.2f)",
                      m_cameraYaw, m_cameraPitch, m_smoothedMouseDelta.x, m_smoothedMouseDelta.y);
    }
}

void CameraController::SetFOV(float FOV)
{
    this->m_radiusFOV = FOV;
}

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

float CameraController::GetCameraYaw() const
{
    return m_cameraYaw;
}

float CameraController::GetCameraPitch() const
{
    return m_cameraPitch;
}

float CameraController::GetCameraSmoothingFactor() const
{
    return m_cameraSmoothingFactor;
}

float CameraController::GetFOV() const
{
    return m_radiusFOV;
}

void CameraController::UpdateMouseRotation(Camera &camera, const Vector3 &playerPosition)
{
    // Skip mouse input if no window is available (for testing)
    if (!IsWindowReady())
    {
        camera.position = {playerPosition.x, playerPosition.y + m_baseCameraY, playerPosition.z};
        camera.target = playerPosition;
        return;
    }

    if (m_cameraMode == CAMERA_FIRST_PERSON)
    {
        // 1st Person: Camera is AT the player position + eye offset
        camera.position = {playerPosition.x, playerPosition.y + m_baseCameraY, playerPosition.z};

        // Direction vectors
        Vector3 forward = {sinf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD),
                           sinf(m_cameraPitch * DEG2RAD),
                           cosf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD)};

        camera.target = Vector3Add(camera.position, forward);
        camera.up = {0.0f, 1.0f, 0.0f};

        // Apply screen shake offset
        if (m_shakeIntensity > 0.0f)
        {
            camera.position = Vector3Add(camera.position, m_shakeOffset);
            camera.target = Vector3Add(camera.target, m_shakeOffset);
        }
    }
    else
    {
        // 3rd Person / Orbital: Camera is shifted by radius
        float currentFOV = GetFOV();
        float wheelMove = GetMouseWheelMove();
        currentFOV -= wheelMove * 0.5f;
        SetFOV(currentFOV);

        if (GetFOV() < 1.0f)
            SetFOV(6.0f);
        if (GetFOV() > 40.0f)
            SetFOV(40.0f);

        Vector3 offset = {GetFOV() * sinf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD),
                          GetFOV() * sinf(m_cameraPitch * DEG2RAD) + 5.0f,
                          GetFOV() * cosf(m_cameraYaw * DEG2RAD) * cosf(m_cameraPitch * DEG2RAD)};

        camera.position = Vector3Add(playerPosition, offset);
        camera.target = playerPosition;

        // Apply screen shake offset
        if (m_shakeIntensity > 0.0f)
        {
            camera.position = Vector3Add(camera.position, m_shakeOffset);
            camera.target = Vector3Add(camera.target, m_shakeOffset);
        }
    }
}

void CameraController::AddScreenShake(float intensity, float duration)
{
    // Add to existing shake if already shaking (stack effect for strong impacts)
    if (m_shakeDuration > 0.0f)
    {
        m_shakeIntensity = std::fmax(m_shakeIntensity, intensity);
        m_shakeDuration = std::fmax(m_shakeDuration, duration);
    }
    else
    {
        m_shakeIntensity = intensity;
        m_shakeDuration = duration;
        m_shakeTimer = 0.0f;
    }
}

void CameraController::UpdateScreenShake(float deltaTime)
{
    if (m_shakeDuration > 0.0f)
    {
        // Update timer
        m_shakeTimer += deltaTime * 30.0f; // Speed of shake animation

        // Calculate shake offset using Perlin-like noise (simple sine waves)
        float shakeAmount = m_shakeIntensity * (m_shakeDuration / 0.5f); // Fade out over time

        m_shakeOffset.x =
            (sinf(m_shakeTimer * 2.1f) + cosf(m_shakeTimer * 1.7f)) * 0.5f * shakeAmount;
        m_shakeOffset.y =
            (sinf(m_shakeTimer * 2.3f) + cosf(m_shakeTimer * 1.9f)) * 0.5f * shakeAmount;
        m_shakeOffset.z =
            (sinf(m_shakeTimer * 1.8f) + cosf(m_shakeTimer * 2.2f)) * 0.5f * shakeAmount;

        // Reduce duration
        m_shakeDuration -= deltaTime;

        if (m_shakeDuration <= 0.0f)
        {
            // Reset shake
            m_shakeIntensity = 0.0f;
            m_shakeDuration = 0.0f;
            m_shakeTimer = 0.0f;
            m_shakeOffset = {0.0f, 0.0f, 0.0f};
        }
    }
}

void CameraController::SetMouseSensitivity(float sensitivity)
{
    m_mouseSensitivity = sensitivity;
}

float CameraController::GetMouseSensitivity() const
{
    return m_mouseSensitivity;
}

CameraController::CameraController(const CameraController &other)
    : m_camera(other.m_camera), m_cameraMode(other.m_cameraMode),
      m_baseCameraY(other.m_baseCameraY), m_cameraYaw(other.m_cameraYaw),
      m_cameraPitch(other.m_cameraPitch), m_cameraSmoothingFactor(other.m_cameraSmoothingFactor),
      m_radiusFOV(other.m_radiusFOV), m_mouseSensitivity(other.m_mouseSensitivity),
      m_smoothedMouseDelta(other.m_smoothedMouseDelta), m_shakeIntensity(other.m_shakeIntensity),
      m_shakeDuration(other.m_shakeDuration), m_shakeTimer(other.m_shakeTimer),
      m_shakeOffset(other.m_shakeOffset)
{
}

CameraController &CameraController::operator=(const CameraController &other)
{
    if (this != &other)
    {
        m_camera = other.m_camera;
        m_cameraMode = other.m_cameraMode;
        m_baseCameraY = other.m_baseCameraY;
        m_cameraYaw = other.m_cameraYaw;
        m_cameraPitch = other.m_cameraPitch;
        m_cameraSmoothingFactor = other.m_cameraSmoothingFactor;
        m_radiusFOV = other.m_radiusFOV;
        m_mouseSensitivity = other.m_mouseSensitivity;
        m_smoothedMouseDelta = other.m_smoothedMouseDelta;
        m_shakeIntensity = other.m_shakeIntensity;
        m_shakeDuration = other.m_shakeDuration;
        m_shakeTimer = other.m_shakeTimer;
        m_shakeOffset = other.m_shakeOffset;
    }
    return *this;
}

void CameraController::OnEvent(CHEngine::Event &e)
{
    CHEngine::EventDispatcher dispatcher(e);

    // Track mouse button state
    dispatcher.Dispatch<CHEngine::MouseButtonPressedEvent>(
        [this](CHEngine::MouseButtonPressedEvent &event)
        {
            if (event.GetMouseButton() == MOUSE_LEFT_BUTTON)
                m_isLMBDown = true;
            if (event.GetMouseButton() == MOUSE_RIGHT_BUTTON)
                m_isRMBDown = true;
            return false;
        });

    dispatcher.Dispatch<CHEngine::MouseButtonReleasedEvent>(
        [this](CHEngine::MouseButtonReleasedEvent &event)
        {
            if (event.GetMouseButton() == MOUSE_LEFT_BUTTON)
                m_isLMBDown = false;
            if (event.GetMouseButton() == MOUSE_RIGHT_BUTTON)
                m_isRMBDown = false;
            return false;
        });

    // Track movement keys
    auto isMovementKey = [](int key)
    {
        return key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT ||
               key == KEY_W || key == KEY_A || key == KEY_S || key == KEY_D || key == KEY_Q ||
               key == KEY_E;
    };

    dispatcher.Dispatch<CHEngine::KeyPressedEvent>(
        [this, isMovementKey](CHEngine::KeyPressedEvent &event)
        {
            if (isMovementKey(event.GetKeyCode()) && event.GetRepeatCount() == 0)
            {
                m_activeMovementKeys++;
            }
            return false;
        });

    dispatcher.Dispatch<CHEngine::KeyReleasedEvent>(
        [this, isMovementKey](CHEngine::KeyReleasedEvent &event)
        {
            if (isMovementKey(event.GetKeyCode()))
            {
                m_activeMovementKeys--;
                if (m_activeMovementKeys < 0)
                    m_activeMovementKeys = 0;
            }
            return false;
        });

    // Track scroll
    dispatcher.Dispatch<CHEngine::MouseScrolledEvent>(
        [this](CHEngine::MouseScrolledEvent &event)
        {
            m_lastMouseWheelMove = event.GetYOffset();
            return false;
        });
}
#include "core/log.h"

