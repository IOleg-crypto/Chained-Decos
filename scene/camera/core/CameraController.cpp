//
// Created by I#Oleg

#include "CameraController.h"

#include "core/events/Event.h"
#include "core/events/KeyEvent.h"
#include "core/events/MouseEvent.h"
#include <cmath>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

CameraController::CameraController() : m_camera({0}), m_cameraMode(CAMERA_THIRD_PERSON)
{
    m_camera.position = (Vector3){4.0f, 4.0f, 4.0f}; // Camera position
    m_camera.target = (Vector3){0.0f, 2.0f, 0.0f};   // Camera looking at point
    m_camera.up = (Vector3){0.0f, 1.0f, 0.0f};       // Camera up vector (rotation towards target)
    m_camera.fovy = 90.0f;                           // Camera field-of-view Y
    m_camera.projection = CAMERA_PERSPECTIVE;        // Camera projection typ {
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
            TraceLog(LOG_WARNING,
                     "CameraController: Mouse delta glitch detected (%.2f, %.2f) - ignored",
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

    // Skip camera update if ImGui wants to capture mouse or keyboard
    // This prevents UpdateCamera from centering the cursor or moving while typing
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard)
    {
        return;
    }

    // Update screen shake
    float deltaTime = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);
    UpdateScreenShake(deltaTime);

    // Only update camera if user is actively interacting with it (Event-driven)
    bool isAnyMovementKeyPressed = m_activeMovementKeys > 0;
    bool isMouseWheelMoving = m_lastMouseWheelMove != 0.0f;

    if (m_isLMBDown || isAnyMovementKeyPressed || isMouseWheelMoving)
    {
        UpdateCamera(&m_camera, m_cameraMode);

        // Reset frame-based states
        m_lastMouseWheelMove = 0.0f;
    }
}

void CameraController::UpdateCameraRotation()
{
    // Don't update if ImGui captured the mouse
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

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
            TraceLog(LOG_WARNING,
                     "CameraController: Mouse position jump detected (%.2f, %.2f) - resetting",
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
        TraceLog(LOG_DEBUG, "CameraController: Manual mouseDelta=(%.2f, %.2f)", mouseDelta.x,
                 mouseDelta.y);
    }

    // Apply centralized filtering to prevent glitches
    Vector2 filteredBefore = mouseDelta;
    mouseDelta = FilterMouseDelta(mouseDelta);

    if (logCounter % 180 == 0 &&
        (filteredBefore.x != mouseDelta.x || filteredBefore.y != mouseDelta.y))
    {
        TraceLog(LOG_DEBUG, "CameraController: Filtered mouseDelta=(%.2f, %.2f) from (%.2f, %.2f)",
                 mouseDelta.x, mouseDelta.y, filteredBefore.x, filteredBefore.y);
    }

    // Smooth movement smoothing using correct lerp coefficient
    const float smoothingFactor = 0.3f; // 0.3 = smooth smoothing
    m_smoothedMouseDelta = Vector2Lerp(m_smoothedMouseDelta, mouseDelta, smoothingFactor);

    // Actual effect on yaw/pitch

    m_cameraYaw -= m_smoothedMouseDelta.x * m_mouseSensitivity;
    m_cameraPitch -= m_smoothedMouseDelta.y * m_mouseSensitivity;

    // Angle clamping using raylib Clamp
    m_cameraPitch = Clamp(m_cameraPitch, -PI / 2.0f + 0.1f, PI / 2.0f - 0.1f);

    if (logCounter % 180 == 0)
    {
        TraceLog(LOG_DEBUG, "CameraController: yaw=%.4f, pitch=%.4f, smoothedDelta=(%.2f, %.2f)",
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

    // Apply screen shake offset
    if (m_shakeIntensity > 0.0f)
    {
        camera.position = Vector3Add(camera.position, m_shakeOffset);
        camera.target = Vector3Add(camera.target, m_shakeOffset);
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

void CameraController::OnEvent(ChainedDecos::Event &e)
{
    ChainedDecos::EventDispatcher dispatcher(e);

    // Track mouse button state
    dispatcher.Dispatch<ChainedDecos::MouseButtonPressedEvent>(
        [this](ChainedDecos::MouseButtonPressedEvent &event)
        {
            if (event.GetMouseButton() == MOUSE_LEFT_BUTTON)
                m_isLMBDown = true;
            return false;
        });

    dispatcher.Dispatch<ChainedDecos::MouseButtonReleasedEvent>(
        [this](ChainedDecos::MouseButtonReleasedEvent &event)
        {
            if (event.GetMouseButton() == MOUSE_LEFT_BUTTON)
                m_isLMBDown = false;
            return false;
        });

    // Track movement keys
    auto isMovementKey = [](int key)
    {
        return key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT ||
               key == KEY_W || key == KEY_A || key == KEY_S || key == KEY_D || key == KEY_Q ||
               key == KEY_E;
    };

    dispatcher.Dispatch<ChainedDecos::KeyPressedEvent>(
        [this, isMovementKey](ChainedDecos::KeyPressedEvent &event)
        {
            if (isMovementKey(event.GetKeyCode()) && event.GetRepeatCount() == 0)
            {
                m_activeMovementKeys++;
            }
            return false;
        });

    dispatcher.Dispatch<ChainedDecos::KeyReleasedEvent>(
        [this, isMovementKey](ChainedDecos::KeyReleasedEvent &event)
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
    dispatcher.Dispatch<ChainedDecos::MouseScrolledEvent>(
        [this](ChainedDecos::MouseScrolledEvent &event)
        {
            m_lastMouseWheelMove = event.GetYOffset();
            return false;
        });
}
