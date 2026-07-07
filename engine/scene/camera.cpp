#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained {

    // Builds a perspective projection matrix from vertical FOV, aspect ratio, and clip planes.
    void Camera::SetPerspective(float verticalFov, float nearClip, float farClip) {
        m_ProjectionType = ProjectionType::Perspective;
        m_PerspectiveFOV = verticalFov;
        m_PerspectiveNear = nearClip;
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    // Builds an orthographic projection matrix from view half-height, aspect ratio, and clip planes.
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

    // Recalculates the projection matrix based on the current projection type,
    // aspect ratio, FOV (perspective), or half-height (orthographic).
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

    // Extracts a Camera3D struct from the camera component and its world transform.
    // Used by the rendering pipeline to get view/projection matrices and position.
    Camera3D Camera::GetCamera3D(const glm::mat4& transform) const {
        Camera3D c;
        c.Position = transform[3];
        glm::vec3 forward = -glm::vec3(transform[2]);
        c.Target = c.Position + forward;
        c.Up = glm::vec3(transform[1]);
        c.Projection = (int)m_ProjectionType;
        c.FovY = glm::degrees(m_PerspectiveFOV);
        c.NearClip = (m_ProjectionType == ProjectionType::Perspective) ? m_PerspectiveNear : m_OrthographicNear;
        c.FarClip = (m_ProjectionType == ProjectionType::Perspective) ? m_PerspectiveFar : m_OrthographicFar;
        c.ProjectionMatrix = m_Projection;
        c.ViewMatrix = glm::inverse(transform);
        return c;
    }
}
