#include "editor_camera.h"
#include "engine/core/input.h"

namespace CHEngine
{

EditorCamera::EditorCamera()
{
    m_Camera.position = {10.0f, 10.0f, 10.0f};
    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.up = {0.0f, 1.0f, 0.0f};
    m_Camera.fovy = 45.0f;
    m_Camera.projection = CAMERA_PERSPECTIVE;
}

void EditorCamera::OnUpdate(float deltaTime)
{
    if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        UpdateCamera(&m_Camera, CAMERA_FREE);
    }
}

void EditorCamera::SetPosition(Vector3 pos)
{
    m_Camera.position = pos;
}
void EditorCamera::SetTarget(Vector3 target)
{
    m_Camera.target = target;
}
void EditorCamera::SetFOV(float fov)
{
    m_Camera.fovy = fov;
}
Camera3D &EditorCamera::GetRaylibCamera()
{
    return m_Camera;
}
const Camera3D &EditorCamera::GetRaylibCamera() const
{
    return m_Camera;
}
} // namespace CHEngine
