#include "scene_camera.h"

namespace Chained
{

void SceneCamera::SetPerspective(float verticalFov, float nearClip, float farClip)
{
    Type = ProjectionType::Perspective;
    PerspectiveFOV = verticalFov;
    PerspectiveNear = nearClip;
    PerspectiveFar = farClip;
}

void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
{
    Type = ProjectionType::Orthographic;
    OrthographicSize = size;
    OrthographicNear = nearClip;
    OrthographicFar = farClip;
}

} // namespace Chained
