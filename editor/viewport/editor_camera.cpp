#include "editor_camera.h"
#include "engine/core/input.h"
#include "engine/scene/project.h"
#include "raymath.h"
#include <cmath>

namespace CHEngine
{

EditorCamera::EditorCamera()
{
    m_Camera.position = {10.0f, 10.0f, 10.0f};
    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.up = {0.0f, 1.0f, 0.0f};
    m_Camera.fovy = 45.0f;
    m_Camera.projection = CAMERA_PERSPECTIVE;

    // Initialize angles based on initial position/target
    Vector3 dir = Vector3Subtract(m_Camera.target, m_Camera.position);
    m_Yaw = atan2f(dir.x, -dir.z); // Z- is forward (Yaw 0)
    m_Pitch = asinf(dir.y / fmaxf(0.001f, Vector3Length(dir)));
}

void EditorCamera::OnUpdate(float deltaTime)
{
    // Sync from project settings if available
    float moveSpeed = m_MoveSpeed;
    float rotationSpeed = m_RotationSpeed;
    float boostMultiplier = m_BoostMultiplier;

    if (auto project = Project::GetActive())
    {
        moveSpeed = project->GetConfig().Editor.CameraMoveSpeed;
        rotationSpeed = project->GetConfig().Editor.CameraRotationSpeed;
        boostMultiplier = project->GetConfig().Editor.CameraBoostMultiplier;
    }

    if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        // 1. Rotation (Mouse Look)
        Vector2 delta = Input::GetMouseDelta();
        m_Yaw += delta.x * rotationSpeed * deltaTime; // Increase yaw when moving mouse right
        m_Pitch -= delta.y * rotationSpeed * deltaTime;

        // Clamp pitch to avoid flipping
        if (m_Pitch > 1.5f) m_Pitch = 1.5f;
        if (m_Pitch < -1.5f) m_Pitch = -1.5f;

        // 2. Movement
        float speed = moveSpeed * deltaTime;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT))
            speed *= boostMultiplier;

        // Correct Vectors for Right-Handed System (Z- is Forward)
        Vector3 forward = { sinf(m_Yaw) * cosf(m_Pitch), sinf(m_Pitch), -cosf(m_Yaw) * cosf(m_Pitch) };
        Vector3 right = { cosf(m_Yaw), 0.0f, sinf(m_Yaw) };
        Vector3 up = { 0.0f, 1.0f, 0.0f };

        if (Input::IsKeyDown(KEY_W)) m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(forward, speed));
        if (Input::IsKeyDown(KEY_S)) m_Camera.position = Vector3Subtract(m_Camera.position, Vector3Scale(forward, speed));
        if (Input::IsKeyDown(KEY_D)) m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(right, speed));
        if (Input::IsKeyDown(KEY_A)) m_Camera.position = Vector3Subtract(m_Camera.position, Vector3Scale(right, speed));
        
        // Vertical movement
        if (Input::IsKeyDown(KEY_E)) m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(up, speed));
        if (Input::IsKeyDown(KEY_Q)) m_Camera.position = Vector3Subtract(m_Camera.position, Vector3Scale(up, speed));

        // Update target to look forward from position
        m_Camera.target = Vector3Add(m_Camera.position, forward);
    }

    // Zoom (Mouse Wheel) - optional but nice
    float wheel = Input::GetMouseWheelMove();
    if (wheel != 0)
    {
        Vector3 dir = Vector3Subtract(m_Camera.target, m_Camera.position);
        float dist = Vector3Length(dir);
        m_Camera.position = Vector3Add(m_Camera.position, Vector3Scale(Vector3Normalize(dir), wheel * 2.0f));
        // Note: target stays same to zoom INTO the point
    }
}

void EditorCamera::SetPosition(Vector3 pos)
{
    m_Camera.position = pos;
    // Re-sync angles
    Vector3 dir = Vector3Subtract(m_Camera.target, m_Camera.position);
    m_Yaw = atan2f(dir.x, -dir.z);
    m_Pitch = asinf(dir.y / fmaxf(0.001f, Vector3Length(dir)));
}

void EditorCamera::SetTarget(Vector3 target)
{
    m_Camera.target = target;
    // Re-sync angles
    Vector3 dir = Vector3Subtract(m_Camera.target, m_Camera.position);
    m_Yaw = atan2f(dir.x, -dir.z);
    m_Pitch = asinf(dir.y / fmaxf(0.001f, Vector3Length(dir)));
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
