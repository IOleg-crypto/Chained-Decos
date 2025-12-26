#ifndef EDITOR_CAMERA_H
#define EDITOR_CAMERA_H

#include "events/Event.h"
#include "events/MouseEvent.h"
#include <raylib.h>
#include <raymath.h>
#include <utility>

namespace CHEngine
{
class EditorCamera
{
public:
    EditorCamera();
    EditorCamera(float fov, float nearClip, float farClip);

    void OnUpdate(float deltaTime);
    void OnEvent(Event &e);

    inline float GetDistance() const
    {
        return m_Distance;
    }
    inline void SetDistance(float distance)
    {
        m_Distance = distance;
    }

    inline void SetViewportSize(float width, float height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
    }

    const Camera3D &GetCamera() const
    {
        return m_Camera;
    }

    Vector3 GetUpDirection() const;
    Vector3 GetRightDirection() const;
    Vector3 GetForwardDirection() const;
    const Vector3 &GetPosition() const
    {
        return m_Camera.position;
    }

    float GetPitch() const
    {
        return m_Pitch;
    }
    float GetYaw() const
    {
        return m_Yaw;
    }

private:
    void UpdateCameraData();

    bool OnMouseScroll(MouseScrolledEvent &e);

    void MousePan(const Vector2 &delta);
    void MouseRotate(const Vector2 &delta);
    void MouseZoom(float delta);

    Vector3 CalculatePosition() const;

    std::pair<float, float> PanSpeed() const;
    float RotationSpeed() const;
    float ZoomSpeed() const;

private:
    Camera3D m_Camera;
    float m_FOV = 90.0f, m_NearClip = 0.1f, m_FarClip = 3000.0f;

    Vector3 m_FocalPoint = {0.0f, 0.0f, 0.0f};

    float m_Distance = 7.0f;
    float m_Pitch = 0.4f, m_Yaw = 0.0f;

    float m_ViewportWidth = 1280, m_ViewportHeight = 720;

    Vector2 m_InitialMousePos = {0.0f, 0.0f};
    bool m_Interacting = false;
    bool m_MousePressedLastFrame = false;
};
} // namespace CHEngine

#endif // EDITOR_CAMERA_H
