#include "editor_camera.h"

namespace CHEngine
{

EditorCamera::EditorCamera()
    : SceneCamera()
{
    UpdateView();
}

EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
{
    SetPerspective(fov, nearClip, farClip);
    SetViewportSize((uint32_t)(aspectRatio * 1000.0f), 1000);
    UpdateView();
}

void EditorCamera::OnUpdate(Timestep ts)
{
    UpdateView();
}

void EditorCamera::UpdateView()
{
    Vector3 position = CalculatePosition();
    m_ViewMatrix = MatrixLookAt(position, Vector3Add(position, GetForwardDirection()), GetUpDirection());
}

Vector3 EditorCamera::GetUpDirection() const
{
    return Vector3RotateByQuaternion({0.0f, 1.0f, 0.0f}, GetOrientation());
}

Vector3 EditorCamera::GetRightDirection() const
{
    return Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, GetOrientation());
}

Vector3 EditorCamera::GetForwardDirection() const
{
    return Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, GetOrientation());
}

Vector3 EditorCamera::CalculatePosition() const
{
    return Vector3Subtract(m_FocalPoint, Vector3Scale(GetForwardDirection(), m_Distance));
}

Quaternion EditorCamera::GetOrientation() const
{
    return QuaternionFromEuler(m_Pitch, m_Yaw, 0.0f);
}

void EditorCamera::MousePan(const Vector2& delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint = Vector3Add(m_FocalPoint, Vector3Scale(GetRightDirection(), -delta.x * xSpeed * m_Distance));
    m_FocalPoint = Vector3Add(m_FocalPoint, Vector3Scale(GetUpDirection(), delta.y * ySpeed * m_Distance));
    UpdateView();
}

void EditorCamera::MouseRotate(const Vector2& delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw -= yawSign * delta.x * RotationSpeed();
    m_Pitch -= delta.y * RotationSpeed();
    UpdateView();
}

void EditorCamera::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
    if (m_Distance < 0.1f)
    {
        m_FocalPoint = Vector3Add(m_FocalPoint, GetForwardDirection());
        m_Distance = 0.1f;
    }
    UpdateView();
}

std::pair<float, float> EditorCamera::PanSpeed() const
{
    float x = fminf(m_ViewportWidth / 1000.0f, 2.4f); 
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y = fminf(m_ViewportHeight / 1000.0f, 2.4f); 
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

float EditorCamera::RotationSpeed() const
{
    return 0.8f * DEG2RAD;
}

float EditorCamera::ZoomSpeed() const
{
    float distance = m_Distance * 0.2f;
    distance = fmaxf(distance, 0.0f);
    float speed = distance * distance;
    speed = fminf(speed, 100.0f); // max speed = 100
    return speed;
}

} // namespace CHEngine
