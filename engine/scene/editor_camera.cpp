#include "editor_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

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
    glm::vec3 position = CalculatePosition();
    m_ViewMatrix = glm::lookAt(position, position + GetForwardDirection(), GetUpDirection());
}

glm::vec3 EditorCamera::GetUpDirection() const
{
    return GetOrientation() * glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 EditorCamera::GetRightDirection() const
{
    return GetOrientation() * glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 EditorCamera::GetForwardDirection() const
{
    return GetOrientation() * glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 EditorCamera::CalculatePosition() const
{
    return m_FocalPoint - GetForwardDirection() * m_Distance;
}

glm::quat EditorCamera::GetOrientation() const
{
    return glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f));
}

void EditorCamera::MousePan(const glm::vec2& delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint += GetRightDirection() * (-delta.x * xSpeed * m_Distance);
    m_FocalPoint += GetUpDirection() * (delta.y * ySpeed * m_Distance);
    UpdateView();
}

void EditorCamera::MouseRotate(const glm::vec2& delta)
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
        m_FocalPoint += GetForwardDirection();
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
    return 0.8f * 0.0174532925f; // DEG2RAD
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
