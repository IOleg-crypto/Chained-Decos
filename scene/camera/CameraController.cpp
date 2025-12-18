#include "CameraController.h"
#include <cmath>

namespace Scene
{

CameraController::CameraController()
    : m_mode(0), m_yaw(0.0f), m_pitch(0.0f), m_radius_fov(8.0f), m_mouse_sensitivity(0.1f),
      m_smoothing(4.0f), m_base_y(4.5f), m_shake_intensity(0.0f), m_shake_duration(0.0f),
      m_shake_offset{0, 0, 0}
{

    m_camera = {};
    m_camera.position = {0.0f, 10.0f, 10.0f};
    m_camera.target = {0.0f, 0.0f, 0.0f};
    m_camera.up = {0.0f, 1.0f, 0.0f};
    m_camera.fovy = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
}

void CameraController::Update()
{
    float dt = GetFrameTime();
    UpdateShake(dt);
}

void CameraController::UpdateRotation()
{
    Vector2 delta = GetFilteredMouseDelta();

    m_yaw += delta.x * m_mouse_sensitivity;
    m_pitch += delta.y * m_mouse_sensitivity;

    // Clamp pitch
    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;
}

void CameraController::UpdateOrbit(const Vector3 &target_position)
{
    // Calculate position based on yaw/pitch/radius
    float yaw_rad = m_yaw * DEG2RAD;
    float pitch_rad = m_pitch * DEG2RAD;

    // Calculate offset from target
    Vector3 offset;
    offset.x = sinf(yaw_rad) * cosf(pitch_rad) * m_radius_fov;
    offset.y = sinf(pitch_rad) * m_radius_fov;
    offset.z = cosf(yaw_rad) * cosf(pitch_rad) * m_radius_fov;

    // Update camera
    m_camera.position = Vector3Add(target_position, offset);
    m_camera.position = Vector3Add(m_camera.position, m_shake_offset);
    m_camera.target = target_position;
}

Camera3D &CameraController::GetCamera()
{
    return m_camera;
}

const Camera3D &CameraController::GetCamera() const
{
    return m_camera;
}

void CameraController::SetMode(int mode)
{
    m_mode = mode;
}

int CameraController::GetMode() const
{
    return m_mode;
}

void CameraController::SetFOV(float fov)
{
    m_radius_fov = fov;
}

float CameraController::GetFOV() const
{
    return m_radius_fov;
}

void CameraController::SetSensitivity(float sensitivity)
{
    m_mouse_sensitivity = sensitivity;
}

float CameraController::GetSensitivity() const
{
    return m_mouse_sensitivity;
}

void CameraController::AddShake(float intensity, float duration)
{
    m_shake_intensity = intensity;
    m_shake_duration = duration;
}

void CameraController::UpdateShake(float dt)
{
    if (m_shake_duration > 0)
    {
        m_shake_duration -= dt;

        float x = ((float)GetRandomValue(-100, 100) / 100.0f) * m_shake_intensity;
        float y = ((float)GetRandomValue(-100, 100) / 100.0f) * m_shake_intensity;
        float z = ((float)GetRandomValue(-100, 100) / 100.0f) * m_shake_intensity;

        m_shake_offset = {x, y, z};

        if (m_shake_duration <= 0)
        {
            m_shake_intensity = 0;
            m_shake_offset = {0, 0, 0};
        }
    }
}

Vector2 CameraController::GetFilteredMouseDelta()
{
    Vector2 delta = ::GetMouseDelta();

    if (fabs(delta.x) < 0.1f)
        delta.x = 0;
    if (fabs(delta.y) < 0.1f)
        delta.y = 0;

    return delta;
}

CameraController::CameraController(const CameraController &other)
    : m_mode(other.m_mode), m_yaw(other.m_yaw), m_pitch(other.m_pitch),
      m_radius_fov(other.m_radius_fov), m_mouse_sensitivity(other.m_mouse_sensitivity),
      m_smoothing(other.m_smoothing), m_base_y(other.m_base_y),
      m_shake_intensity(other.m_shake_intensity), m_shake_duration(other.m_shake_duration),
      m_shake_offset(other.m_shake_offset)
{
    m_camera = other.m_camera;
}

CameraController &CameraController::operator=(const CameraController &other)
{
    if (this != &other)
    {
        m_mode = other.m_mode;
        m_yaw = other.m_yaw;
        m_pitch = other.m_pitch;
        m_radius_fov = other.m_radius_fov;
        m_mouse_sensitivity = other.m_mouse_sensitivity;
        m_smoothing = other.m_smoothing;
        m_base_y = other.m_base_y;
        m_shake_intensity = other.m_shake_intensity;
        m_shake_duration = other.m_shake_duration;
        m_shake_offset = other.m_shake_offset;
        m_camera = other.m_camera;
    }
    return *this;
}

} // namespace Scene
