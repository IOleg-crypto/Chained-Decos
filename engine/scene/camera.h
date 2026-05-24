#ifndef CH_CAMERA_H
#define CH_CAMERA_H

#include "engine/core/reflection.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace CHEngine
{

enum class ProjectionType
{
    Perspective = 0,
    Orthographic = 1
};

class Camera
{
public:
    Camera() = default;
    Camera(const glm::mat4& projection) : ProjectionMatrix(projection) {}
    virtual ~Camera() = default;

    const glm::mat4& GetProjection() const { return ProjectionMatrix; }

    virtual void SetViewportSize(uint32_t width, uint32_t height)
    {
        ViewportWidth = width;
        ViewportHeight = height;
        AspectRatio = (float)width / (float)height;
        RecalculateProjection();
    }

    void SetPerspective(float verticalFov, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);

    ProjectionType GetProjectionType() const { return Type; }
    void SetProjectionType(ProjectionType type) { Type = type; RecalculateProjection(); }

    float GetPerspectiveVerticalFOV() const { return PerspectiveFOV; }
    void SetPerspectiveVerticalFOV(float fov) { PerspectiveFOV = fov; RecalculateProjection(); }
    float GetPerspectiveNearClip() const { return PerspectiveNear; }
    void SetPerspectiveNearClip(float nearClip) { PerspectiveNear = nearClip; RecalculateProjection(); }
    float GetPerspectiveFarClip() const { return PerspectiveFar; }
    void SetPerspectiveFarClip(float farClip) { PerspectiveFar = farClip; RecalculateProjection(); }

    float GetOrthographicSize() const { return OrthographicSize; }
    void SetOrthographicSize(float size) { OrthographicSize = size; RecalculateProjection(); }
    float GetOrthographicNearClip() const { return OrthographicNear; }
    void SetOrthographicNearClip(float nearClip) { OrthographicNear = nearClip; RecalculateProjection(); }
    float GetOrthographicFarClip() const { return OrthographicFar; }
    void SetOrthographicFarClip(float farClip) { OrthographicFar = farClip; RecalculateProjection(); }

public:
    glm::mat4 ProjectionMatrix = glm::mat4(1.0f);

    ProjectionType Type = ProjectionType::Perspective;

    float PerspectiveFOV = glm::radians(45.0f);
    float PerspectiveNear = 0.01f;
    float PerspectiveFar = 1000.0f;

    float OrthographicSize = 10.0f;
    float OrthographicNear = -1.0f;
    float OrthographicFar = 1.0f;

    float AspectRatio = 1.778f;
    uint32_t ViewportWidth = 1280;
    uint32_t ViewportHeight = 720;

protected:
    virtual void RecalculateProjection();

    CH_REFLECT_BEGIN(Camera)
        CH_PROP(props, Type);
        CH_PROP(props, PerspectiveFOV);
        CH_PROP(props, PerspectiveNear);
        CH_PROP(props, PerspectiveFar);
        CH_PROP(props, OrthographicSize);
        CH_PROP(props, OrthographicNear);
        CH_PROP(props, OrthographicFar);
        CH_PROP(props, AspectRatio);
        CH_PROP(props, ViewportWidth);
        CH_PROP(props, ViewportHeight);

        if (props.HasChanged() || props.GetMode() == ReflectionMode::Deserialize)
        {
            RecalculateProjection();
        }
    CH_REFLECT_END()

private:
};

} // namespace CHEngine

#endif // CH_CAMERA_H
