#ifndef CH_EDITOR_CAMERA_H
#define CH_EDITOR_CAMERA_H

#include "engine/core/timestep.h"
#include "engine/scene/entity.h"
#include "raylib.h"

namespace CHEngine
{

class EditorCameraController
{
public:
    EditorCameraController();
    ~EditorCameraController() = default;

    // Drives the transform and camera component of the given entity
    void OnUpdate(Entity cameraEntity, Timestep ts);

    float GetYaw() const
    {
        return m_Yaw;
    }
    float GetPitch() const
    {
        return m_Pitch;
    }

private:
    void MouseRotate(const Vector2& delta);
    void MousePan(const Vector2& delta);
    void MouseZoom(float delta);

    Vector3 GetUpDirection() const;
    Vector3 GetRightDirection() const;
    Vector3 GetForwardDirection() const;

    Vector3 CalculatePosition() const;
    std::pair<float, float> PanSpeed() const;
    float RotationSpeed() const;
    float ZoomSpeed() const;

private:
    float m_MoveSpeed = 10.0f;
    float m_RotationSpeed = 0.002f;
    float m_BoostMultiplier = 5.0f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    Vector3 m_FocalPoint = {0.0f, 0.0f, 0.0f};
    float m_Distance = 10.0f;

    Vector2 m_InitialMousePosition = {0.0f, 0.0f};
    uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
};

} // namespace CHEngine

#endif // CH_EDITOR_CAMERA_H
