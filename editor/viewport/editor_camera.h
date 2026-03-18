#ifndef CH_EDITOR_CAMERA_H
#define CH_EDITOR_CAMERA_H

#include "engine/core/timestep.h"
#include "engine/scene/entity.h"
#include "engine/scene/editor_camera.h"

namespace CHEngine
{

class EditorCameraController
{
public:
    EditorCameraController();
    ~EditorCameraController() = default;

    // Drives the transform and camera component of the given entity
    void OnUpdate(Entity cameraEntity, Timestep ts);

    EditorCamera& GetCamera() { return m_Camera; }
    float GetYaw() const { return m_Camera.GetYaw(); }
    float GetPitch() const { return m_Camera.GetPitch(); }

private:
    float m_MoveSpeed = 10.0f;
    float m_BoostMultiplier = 5.0f;

    EditorCamera m_Camera;
    Vector2 m_InitialMousePosition = {0.0f, 0.0f};
};

} // namespace CHEngine

#endif // CH_EDITOR_CAMERA_H

