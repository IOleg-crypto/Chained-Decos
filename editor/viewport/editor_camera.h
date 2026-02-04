#ifndef CH_EDITOR_CAMERA_H
#define CH_EDITOR_CAMERA_H

#include "raylib.h"
#include "engine/core/timestep.h"

namespace CHEngine
{

class EditorCamera
{
public:
    EditorCamera();
    ~EditorCamera() = default;

    void OnUpdate(Timestep ts);

    const Camera3D &GetRaylibCamera() const;
    Camera3D &GetRaylibCamera();

    void SetPosition(Vector3 pos);
    void SetTarget(Vector3 target);
    void SetFOV(float fov);

private:
    Camera3D m_Camera;
    float m_MoveSpeed = 10.0f;
    float m_RotationSpeed = 0.1f;
    float m_BoostMultiplier = 5.0f;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
};

} // namespace CHEngine

#endif // CH_EDITOR_CAMERA_H
