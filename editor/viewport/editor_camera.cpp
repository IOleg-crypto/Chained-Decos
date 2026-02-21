#include "editor_camera.h"
#include "editor/editor_layer.h"
#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "raymath.h"
#include <cmath>

namespace CHEngine
{

EditorCameraController::EditorCameraController()
{
    m_FocalPoint = {0.0f, 0.0f, 0.0f};
    m_Distance = 10.0f;
    m_Yaw = 0.0f;
    m_Pitch = 0.0f;
}

void EditorCameraController::OnUpdate(Entity cameraEntity, Timestep ts)
{
    if (!cameraEntity || !cameraEntity.HasComponent<TransformComponent>() ||
        !cameraEntity.HasComponent<CameraComponent>())
    {
        return;
    }

    auto& tc = cameraEntity.GetComponent<TransformComponent>();
    float deltaTime = ts;

    // Viewport dimensions for calculations
    m_ViewportWidth = (uint32_t)EditorLayer::Get().GetViewportSize().x;
    m_ViewportHeight = (uint32_t)EditorLayer::Get().GetViewportSize().y;

    // 1. Sync from current transform if it was changed externally (e.g. Inspector)
    if (fabsf(tc.Rotation.x - m_Pitch * RAD2DEG) > 0.01f || fabsf(tc.Rotation.y - m_Yaw * RAD2DEG) > 0.01f)
    {
        m_Pitch = tc.Rotation.x * DEG2RAD;
        m_Yaw = tc.Rotation.y * DEG2RAD;

        // If position also changed, we might need to update focal point
        // But for now, let's just update the angles to prevent snapping
    }

    const Vector2& mouse = Input::GetMousePosition();
    Vector2 delta = Input::GetMouseDelta();
    m_InitialMousePosition = mouse;

    if (Input::IsKeyDown(KEY_LEFT_ALT))
    {
        if (Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            MouseRotate(delta);
        }
        else if (Input::IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            MousePan(delta);
        }
        else if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            MouseZoom(delta.y);
        }
    }
    else if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        // Fly mode (FPS style)
        MouseRotate(delta);

        float speed = m_MoveSpeed * deltaTime;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT))
        {
            speed *= m_BoostMultiplier;
        }

        Vector3 forward = GetForwardDirection();
        Vector3 right = GetRightDirection();
        Vector3 up = {0, 1, 0};

        if (Input::IsKeyDown(KEY_W))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(forward, speed));
        }
        if (Input::IsKeyDown(KEY_S))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(forward, speed));
        }
        if (Input::IsKeyDown(KEY_D))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(right, speed));
        }
        if (Input::IsKeyDown(KEY_A))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(right, speed));
        }
        if (Input::IsKeyDown(KEY_E))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(up, speed));
        }
        if (Input::IsKeyDown(KEY_Q))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(up, speed));
        }

        // In fly mode, focal point follows position at fixed distance
        m_FocalPoint = Vector3Add(tc.Translation, Vector3Scale(forward, m_Distance));
    }

    float wheel = Input::GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseZoom(wheel);
    }

    // Update Transform logic
    tc.RotationQuat = QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f);
    tc.Rotation.x = m_Pitch * RAD2DEG;
    tc.Rotation.y = m_Yaw * RAD2DEG;
    tc.Rotation.z = 0.0f;

    if (!Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        // Only drive position from focal point if NOT in fly mode
        tc.Translation = CalculatePosition();
    }
}

void EditorCameraController::MouseRotate(const Vector2& delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw -= yawSign * delta.x * RotationSpeed();
    m_Pitch -= delta.y * RotationSpeed();
}

void EditorCameraController::MousePan(const Vector2& delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint = Vector3Add(m_FocalPoint, Vector3Scale(GetRightDirection(), -delta.x * xSpeed * m_Distance));
    m_FocalPoint = Vector3Add(m_FocalPoint, Vector3Scale(GetUpDirection(), delta.y * ySpeed * m_Distance));
}

void EditorCameraController::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
    if (m_Distance < 0.1f)
    {
        m_FocalPoint = Vector3Add(m_FocalPoint, GetForwardDirection());
        m_Distance = 0.1f;
    }
}

Vector3 EditorCameraController::GetUpDirection() const
{
    return Vector3RotateByQuaternion({0.0f, 1.0f, 0.0f}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

Vector3 EditorCameraController::GetRightDirection() const
{
    return Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

Vector3 EditorCameraController::GetForwardDirection() const
{
    return Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

Vector3 EditorCameraController::CalculatePosition() const
{
    return Vector3Subtract(m_FocalPoint, Vector3Scale(GetForwardDirection(), m_Distance));
}

std::pair<float, float> EditorCameraController::PanSpeed() const
{
    float x = fminf(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y = fminf(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

float EditorCameraController::RotationSpeed() const
{
    return 0.8f * DEG2RAD; // Constant for now, can be mouse sensitivity
}

float EditorCameraController::ZoomSpeed() const
{
    float distance = m_Distance * 0.2f;
    distance = fmaxf(distance, 0.0f);
    float speed = distance * distance;
    speed = fminf(speed, 100.0f); // max speed = 100
    return speed;
}

} // namespace CHEngine
