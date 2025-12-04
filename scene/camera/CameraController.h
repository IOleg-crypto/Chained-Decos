#ifndef SCENE_CAMERA_CONTROLLER_H
#define SCENE_CAMERA_CONTROLLER_H

#include <raylib.h>
#include <raymath.h>

namespace Scene
{

class CameraController
{
public:
    CameraController();
    ~CameraController() = default;

    // Non-copyable
    CameraController(const CameraController &) = delete;
    CameraController &operator=(const CameraController &) = delete;

    // Core
    void Update();
    void UpdateRotation();
    void UpdateOrbit(const Vector3 &target_position);

    // Accessors
    Camera3D &GetCamera();
    const Camera3D &GetCamera() const;

    // Settings
    void SetMode(int mode);
    int GetMode() const;

    void SetFOV(float fov);
    float GetFOV() const;

    void SetSensitivity(float sensitivity);
    float GetSensitivity() const;

    // Effects
    void AddShake(float intensity, float duration = 0.5f);

private:
    void UpdateShake(float dt);
    static Vector2 GetFilteredMouseDelta();

    Camera3D m_camera;
    int m_mode;

    // Camera params
    float m_yaw;
    float m_pitch;
    float m_radius_fov;
    float m_mouse_sensitivity;
    float m_smoothing;
    float m_base_y;

    // Shake state
    float m_shake_intensity;
    float m_shake_duration;
    Vector3 m_shake_offset;
};

} // namespace Scene

#endif // SCENE_CAMERA_CONTROLLER_H
