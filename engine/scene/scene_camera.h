#ifndef CH_SCENE_CAMERA_H
#define CH_SCENE_CAMERA_H

#include <glm/glm.hpp>
#include <cstdint>

namespace Chained
{

enum class ProjectionType
{
    Perspective = 0,
    Orthographic = 1
};

class SceneCamera
{
public:
    ~SceneCamera() = default;

    void SetPerspective(float verticalFov, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);

    ProjectionType GetProjectionType() const { return Type; }
    void SetProjectionType(ProjectionType type) { Type = type; }

    float GetPerspectiveVerticalFOV() const { return PerspectiveFOV; }
    void SetPerspectiveVerticalFOV(float fov) { PerspectiveFOV = fov; }
    float GetPerspectiveNearClip() const { return PerspectiveNear; }
    void SetPerspectiveNearClip(float nearClip) { PerspectiveNear = nearClip; }
    float GetPerspectiveFarClip() const { return PerspectiveFar; }
    void SetPerspectiveFarClip(float farClip) { PerspectiveFar = farClip; }

    float GetOrthographicSize() const { return OrthographicSize; }
    void SetOrthographicSize(float size) { OrthographicSize = size; }
    float GetOrthographicNearClip() const { return OrthographicNear; }
    void SetOrthographicNearClip(float nearClip) { OrthographicNear = nearClip; }
    float GetOrthographicFarClip() const { return OrthographicFar; }
    void SetOrthographicFarClip(float farClip) { OrthographicFar = farClip; }

public:
    ProjectionType Type = ProjectionType::Perspective;

    float PerspectiveFOV = glm::radians(45.0f);
    float PerspectiveNear = 0.1f;
    float PerspectiveFar = 10000.0f;

    float OrthographicSize = 10.0f;
    float OrthographicNear = -1.0f;
    float OrthographicFar = 1.0f;
};

} // namespace Chained

#endif // CH_SCENE_CAMERA_H
