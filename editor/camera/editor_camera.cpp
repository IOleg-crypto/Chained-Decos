#include "editor_camera.h"
#include "core/Base.h"
#include <algorithm>
#include <imgui.h>

namespace CHEngine
{
EditorCamera::EditorCamera()
{
    m_Camera.position = {10, 10, 10};
    m_Camera.target = {0, 0, 0};
    m_Camera.up = {0, 1, 0};
    m_Camera.fovy = m_FOV;
    m_Camera.projection = CAMERA_PERSPECTIVE;

    UpdateCameraData();
}

EditorCamera::EditorCamera(float fov, float nearClip, float farClip)
    : m_FOV(fov), m_NearClip(nearClip), m_FarClip(farClip)
{
    m_Camera.position = {10, 10, 10};
    m_Camera.target = {0, 0, 0};
    m_Camera.up = {0, 1, 0};
    m_Camera.fovy = m_FOV;
    m_Camera.projection = CAMERA_PERSPECTIVE;

    UpdateCameraData();
}

void EditorCamera::OnUpdate(float deltaTime)
{
    Vector2 mouse = GetMousePosition();
    Vector2 delta = Vector2Subtract(mouse, m_InitialMousePos);

    // Update initial mouse pos for NEXT frame before using it
    // This prevents jumps when mouse was outside viewport and we just started interacting
    m_InitialMousePos = mouse;

    bool isRightMouseDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    bool isLeftMouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool isMiddleMouseDown = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    bool isAltDown = IsKeyDown(KEY_LEFT_ALT);

    // Interaction state management
    bool isInteractingThisFrame = isAltDown || isRightMouseDown;

    if (isInteractingThisFrame)
    {
        if (!m_Interacting)
        {
            m_Interacting = true;
            return; // Skip first frame of interaction to avoid delta jump
        }

        // 1. Orbital Controls (Original)
        if (isAltDown)
        {
            if (isMiddleMouseDown)
                MousePan(delta);
            else if (isLeftMouseDown)
                MouseRotate(delta);
            else if (isRightMouseDown)
                MouseZoom(delta.y);
        }
        // 2. Freelook / Fly Controls (New)
        else if (isRightMouseDown)
        {
            // Right click freelook
            MouseRotate(delta);

            // WASD/QE Movement
            float moveSpeed = 10.0f * deltaTime;
            if (IsKeyDown(KEY_LEFT_SHIFT))
                moveSpeed *= 3.0f; // Turbo

            if (IsKeyDown(KEY_W))
                m_FocalPoint =
                    Vector3Add(m_FocalPoint, Vector3Scale(GetForwardDirection(), moveSpeed));
            if (IsKeyDown(KEY_S))
                m_FocalPoint =
                    Vector3Subtract(m_FocalPoint, Vector3Scale(GetForwardDirection(), moveSpeed));
            if (IsKeyDown(KEY_A))
                m_FocalPoint =
                    Vector3Subtract(m_FocalPoint, Vector3Scale(GetRightDirection(), moveSpeed));
            if (IsKeyDown(KEY_D))
                m_FocalPoint =
                    Vector3Add(m_FocalPoint, Vector3Scale(GetRightDirection(), moveSpeed));
            if (IsKeyDown(KEY_E))
                m_FocalPoint = Vector3Add(m_FocalPoint, Vector3Scale(GetUpDirection(), moveSpeed));
            if (IsKeyDown(KEY_Q))
                m_FocalPoint =
                    Vector3Subtract(m_FocalPoint, Vector3Scale(GetUpDirection(), moveSpeed));
        }
    }
    else
    {
        m_Interacting = false;
    }

    UpdateCameraData();
}

void EditorCamera::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(CD_BIND_EVENT_FN(EditorCamera::OnMouseScroll));
}

bool EditorCamera::OnMouseScroll(MouseScrolledEvent &e)
{
    float delta = e.GetYOffset() * 0.1f;
    MouseZoom(delta);
    UpdateCameraData();
    return false;
}

void EditorCamera::MousePan(const Vector2 &delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint =
        Vector3Add(m_FocalPoint, Vector3Scale(GetRightDirection(), -delta.x * xSpeed * m_Distance));
    m_FocalPoint =
        Vector3Add(m_FocalPoint, Vector3Scale(GetUpDirection(), delta.y * ySpeed * m_Distance));
}

void EditorCamera::MouseRotate(const Vector2 &delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * RotationSpeed();
    m_Pitch += delta.y * RotationSpeed();
}

void EditorCamera::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
    if (m_Distance < 1.0f)
    {
        m_FocalPoint =
            Vector3Add(m_FocalPoint, Vector3Scale(GetForwardDirection(), m_Distance - 1.0f));
        m_Distance = 1.0f;
    }
}

void EditorCamera::UpdateCameraData()
{
    m_Camera.position = CalculatePosition();

    m_Camera.target = m_FocalPoint;
    m_Camera.up = GetUpDirection();
}

Vector3 EditorCamera::CalculatePosition() const
{
    return Vector3Add(m_FocalPoint, Vector3Scale(GetForwardDirection(), -m_Distance));
}

Vector3 EditorCamera::GetUpDirection() const
{
    return Vector3RotateByQuaternion({0, 1, 0}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

Vector3 EditorCamera::GetRightDirection() const
{
    return Vector3RotateByQuaternion({1, 0, 0}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

Vector3 EditorCamera::GetForwardDirection() const
{
    return Vector3RotateByQuaternion({0, 0, -1}, QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f));
}

std::pair<float, float> EditorCamera::PanSpeed() const
{
    float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

float EditorCamera::RotationSpeed() const
{
    return 0.8f * 0.01f;
}

float EditorCamera::ZoomSpeed() const
{
    float distance = m_Distance * 0.2f;
    distance = std::max(distance, 0.0f);
    float speed = distance * distance;
    speed = std::min(speed, 100.0f);
    return speed * 0.01f;
}
} // namespace CHEngine
