#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained {

    void Camera::SetPerspective(float verticalFov, float nearClip, float farClip) {
        m_ProjectionType = ProjectionType::Perspective;
        m_PerspectiveFOV = verticalFov;
        m_PerspectiveNear = nearClip;
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    void Camera::SetOrthographic(float size, float nearClip, float farClip) {
        m_ProjectionType = ProjectionType::Orthographic;
        m_OrthographicSize = size;
        m_OrthographicNear = nearClip;
        m_OrthographicFar = farClip;
        RecalculateProjection();
    }

    void Camera::SetViewportSize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        m_AspectRatio = (float)width / (float)height;
        RecalculateProjection();
    }

    void Camera::RecalculateProjection() {
        if (m_ProjectionType == ProjectionType::Perspective) {
            m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
        } else {
            float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoBottom = -m_OrthographicSize * 0.5f;
            float orthoTop = m_OrthographicSize * 0.5f;
            m_Projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
        }
    }

    Camera3D Camera::GetCamera3D(const glm::mat4& transform) const {
        Camera3D camera;
        camera.Position = transform[3];
        glm::vec3 forward = -glm::vec3(transform[2]);
        camera.Target = camera.Position + forward;
        camera.Up = glm::vec3(transform[1]);
        camera.Projection = m_ProjectionType;
        camera.FovDegrees = glm::degrees(m_PerspectiveFOV);
        camera.OrthographicSize = m_OrthographicSize;
        camera.NearClip = (m_ProjectionType == ProjectionType::Perspective) ? m_PerspectiveNear : m_OrthographicNear;
        camera.FarClip = (m_ProjectionType == ProjectionType::Perspective) ? m_PerspectiveFar : m_OrthographicFar;
        camera.ProjectionMatrix = m_Projection;
        camera.ViewMatrix = glm::inverse(transform);
        return c;
    }

    Camera2D Camera::GetCamera2D(const glm::mat4& transform) const {
        Camera2D camera;
        camera.Position = glm::vec2(transform[3]);
        camera.Rotation = glm::degrees(glm::atan(transform[0][1], transform[0][0]));
        camera.Zoom = 1.0f / m_OrthographicSize;
        camera.NearClip = m_OrthographicNear;
        camera.FarClip = m_OrthographicFar;

        camera.ViewMatrix = glm::inverse(transform);
        camera.ProjectionMatrix = m_Projection;
        return camera;
    }
}
