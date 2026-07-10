#ifndef CH_CAMERA_H
#define CH_CAMERA_H

#include <glm/glm.hpp>
#include "engine/graphics/camera_types.h"

namespace Chained
{
    class Camera
    {
    public:
        using ProjectionType = Chained::ProjectionType;

        Camera() { RecalculateProjection(); }
        virtual ~Camera() = default;

        const glm::mat4& GetProjection() const { return m_Projection; }

        void SetPerspective(float verticalFov, float nearClip, float farClip);
        void SetOrthographic(float size, float nearClip, float farClip);
        void SetViewportSize(uint32_t width, uint32_t height);

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

        Camera3D GetCamera3D(const glm::mat4& transform) const;
        Camera2D GetCamera2D(const glm::mat4& transform) const;

    protected:
        void RecalculateProjection();

    protected:
        glm::mat4 m_Projection{1.0f};

        ProjectionType m_ProjectionType = ProjectionType::Perspective;
        float m_PerspectiveFOV = glm::radians(45.0f);
        float m_PerspectiveNear = 0.1f, m_PerspectiveFar = 10000.0f;

        float m_OrthographicSize = 10.0f;
        float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;
        float m_AspectRatio = 16.0f / 9.0f;
    };
}
#endif // CH_CAMERA_H
