#ifndef CH_ENGINE_EDITOR_CAMERA_H
#define CH_ENGINE_EDITOR_CAMERA_H

#include "engine/core/ch_structures.h"
#include "engine/core/timestep.h"
#include "engine/scene/scene_camera.h"

namespace CHEngine
{

class EditorCamera : public SceneCamera
{
public:
    EditorCamera();
    EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

    void OnUpdate(Timestep ts);

    void SetViewportSize(uint32_t width, uint32_t height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        SceneCamera::SetViewportSize(width, height);
    }

    const glm::mat4& GetViewMatrix() const
    {
        return m_ViewMatrix;
    }
    glm::mat4 GetViewProjection() const
    {
        return GetProjection() * m_ViewMatrix;
    }

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;
    glm::vec3 CalculatePosition() const;
    glm::quat GetOrientation() const;

    float GetPitch() const
    {
        return m_Pitch;
    }
    float GetYaw() const
    {
        return m_Yaw;
    }

    void SetPitch(float pitch)
    {
        m_Pitch = pitch;
    }
    void SetYaw(float yaw)
    {
        m_Yaw = yaw;
    }

    glm::vec3 GetFocalPoint() const
    {
        return m_FocalPoint;
    }
    void SetFocalPoint(const glm::vec3& focalPoint)
    {
        m_FocalPoint = focalPoint;
    }

    float GetDistance() const
    {
        return m_Distance;
    }
    void SetDistance(float distance)
    {
        m_Distance = distance;
    }

    void MousePan(const glm::vec2& delta);
    void MouseRotate(const glm::vec2& delta);
    void MouseZoom(float delta);

    std::pair<float, float> PanSpeed() const;
    float RotationSpeed() const;
    float ZoomSpeed() const;

private:
    void UpdateView();

private:
    float m_Pitch = 0.0f, m_Yaw = 0.0f;
    glm::vec3 m_FocalPoint = {0.0f, 0.0f, 0.0f};
    float m_Distance = 10.0f;

    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
};

} // namespace CHEngine

#endif // CH_ENGINE_EDITOR_CAMERA_H
