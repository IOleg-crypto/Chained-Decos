#include "scene_camera.h"

namespace CHEngine
{

SceneCamera::SceneCamera()
{
    RecalculateProjection();
}

void SceneCamera::SetPerspective(float verticalFov, float nearClip, float farClip)
{
    m_ProjectionType = ProjectionType::Perspective;
    m_PerspectiveFOV = verticalFov;
    m_PerspectiveNear = nearClip;
    m_PerspectiveFar = farClip;
    RecalculateProjection();
}

void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
{
    m_ProjectionType = ProjectionType::Orthographic;
    m_OrthographicSize = size;
    m_OrthographicNear = nearClip;
    m_OrthographicFar = farClip;
    RecalculateProjection();
}

void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
{
    m_AspectRatio = (float)width / (float)height;
    RecalculateProjection();
}

void SceneCamera::RecalculateProjection()
{
    if (m_ProjectionType == ProjectionType::Perspective)
    {
        m_Projection = MatrixPerspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
    }
    else
    {
        float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
        float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
        float orthoBottom = -m_OrthographicSize * 0.5f;
        float orthoTop = m_OrthographicSize * 0.5f;

        m_Projection = MatrixOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
    }
}

} // namespace CHEngine
