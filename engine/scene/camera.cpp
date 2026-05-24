#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace CHEngine
{

void Camera::SetPerspective(float verticalFov, float nearClip, float farClip)
{
    Type = ProjectionType::Perspective;
    PerspectiveFOV = verticalFov;
    PerspectiveNear = nearClip;
    PerspectiveFar = farClip;
    RecalculateProjection();
}

void Camera::SetOrthographic(float size, float nearClip, float farClip)
{
    Type = ProjectionType::Orthographic;
    OrthographicSize = size;
    OrthographicNear = nearClip;
    OrthographicFar = farClip;
    RecalculateProjection();
}

void Camera::RecalculateProjection()
{
    if (Type == ProjectionType::Perspective)
    {
        ProjectionMatrix = glm::perspective(PerspectiveFOV, AspectRatio, PerspectiveNear, PerspectiveFar);
    }
    else
    {
        float orthoLeft = -OrthographicSize * AspectRatio * 0.5f;
        float orthoRight = OrthographicSize * AspectRatio * 0.5f;
        float orthoBottom = -OrthographicSize * 0.5f;
        float orthoTop = OrthographicSize * 0.5f;

        ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, OrthographicNear, OrthographicFar);
    }
}

} // namespace CHEngine
