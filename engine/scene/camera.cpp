#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace CHEngine
{

void Camera::SetPerspective(float verticalFov, float nearClip, float farClip)
{
    m_ProjectionType = ProjectionType::Perspective;
    m_PerspectiveFOV = verticalFov;
    m_PerspectiveNear = nearClip;
    m_PerspectiveFar = farClip;
    RecalculateProjection();
}

void Camera::SetOrthographic(float size, float nearClip, float farClip)
{
    m_ProjectionType = ProjectionType::Orthographic;
    m_OrthographicSize = size;
    m_OrthographicNear = nearClip;
    m_OrthographicFar = farClip;
    RecalculateProjection();
}

void Camera::RecalculateProjection()
{
    if (m_ProjectionType == ProjectionType::Perspective)
    {
        m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
    }
    else
    {
        float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
        float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
        float orthoBottom = -m_OrthographicSize * 0.5f;
        float orthoTop = m_OrthographicSize * 0.5f;

        m_Projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
    }
}

} // namespace CHEngine
