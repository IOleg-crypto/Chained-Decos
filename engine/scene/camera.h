#ifndef CH_CAMERA_H
#define CH_CAMERA_H

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
    Camera(const glm::mat4& projection) : m_Projection(projection) {}
    virtual ~Camera() = default;

    const glm::mat4& GetProjection() const { return m_Projection; }

    virtual void SetViewportSize(uint32_t width, uint32_t height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        m_AspectRatio = (float)width / (float)height;
        RecalculateProjection();
    }

    void SetPerspective(float verticalFov, float nearClip, float farClip);
    void SetOrthographic(float size, float nearClip, float farClip);

    ProjectionType GetProjectionType() const { return m_ProjectionType; }
    void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

    float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
    void SetPerspectiveVerticalFOV(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }
    float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
    void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
    float GetPerspectiveFarClip() const { return m_PerspectiveFar; }
    void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

    float GetOrthographicSize() const { return m_OrthographicSize; }
    void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
    float GetOrthographicNearClip() const { return m_OrthographicNear; }
    void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
    float GetOrthographicFarClip() const { return m_OrthographicFar; }
    void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

protected:
    virtual void RecalculateProjection();

protected:
    glm::mat4 m_Projection = glm::mat4(1.0f);

    ProjectionType m_ProjectionType = ProjectionType::Perspective;

    float m_PerspectiveFOV = glm::radians(45.0f);
    float m_PerspectiveNear = 0.01f;
    float m_PerspectiveFar = 1000.0f;

    float m_OrthographicSize = 10.0f;
    float m_OrthographicNear = -1.0f;
    float m_OrthographicFar = 1.0f;

    float m_AspectRatio = 1.778f;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
};

} // namespace CHEngine

#endif // CH_CAMERA_H
