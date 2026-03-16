#ifndef CH_ENGINE_EDITOR_CAMERA_H
#define CH_ENGINE_EDITOR_CAMERA_H

#include "engine/scene/scene_camera.h"
#include "engine/core/timestep.h"
#include "raylib.h"
#include "raymath.h"

namespace CHEngine
{

class EditorCamera : public SceneCamera
{
public:
    EditorCamera();
    EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

    void OnUpdate(Timestep ts); // Generic update if needed, though usually driven by controller
    
    void SetViewportSize(uint32_t width, uint32_t height) 
    { 
        m_ViewportWidth = width; m_ViewportHeight = height; 
        SceneCamera::SetViewportSize(width, height); 
    }

    const Matrix& GetViewMatrix() const { return m_ViewMatrix; }
    Matrix GetViewProjection() const { return MatrixMultiply(m_ViewMatrix, GetProjection()); }

    Vector3 GetUpDirection() const;
    Vector3 GetRightDirection() const;
    Vector3 GetForwardDirection() const;
    Vector3 CalculatePosition() const;
    Quaternion GetOrientation() const;

    float GetPitch() const { return m_Pitch; }
    float GetYaw() const { return m_Yaw; }
    
    void SetPitch(float pitch) { m_Pitch = pitch; }
    void SetYaw(float yaw) { m_Yaw = yaw; }

    Vector3 GetFocalPoint() const { return m_FocalPoint; }
    void SetFocalPoint(const Vector3& focalPoint) { m_FocalPoint = focalPoint; }
    
    float GetDistance() const { return m_Distance; }
    void SetDistance(float distance) { m_Distance = distance; }

    // Transformation helpers used by controllers
    void MousePan(const Vector2& delta);
    void MouseRotate(const Vector2& delta);
    void MouseZoom(float delta);

    // Speed helpers
    std::pair<float, float> PanSpeed() const;
    float RotationSpeed() const;
    float ZoomSpeed() const;

private:
    void UpdateView();

private:
    float m_Pitch = 0.0f, m_Yaw = 0.0f;
    Vector3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
    float m_Distance = 10.0f;

    Matrix m_ViewMatrix = MatrixIdentity();
    uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
};

} // namespace CHEngine

#endif // CH_ENGINE_EDITOR_CAMERA_H
