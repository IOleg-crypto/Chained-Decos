#include "editor_camera.h"
#include "editor/editor_layer.h"
#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"

namespace CHEngine
{

EditorCameraController::EditorCameraController()
{
}

void EditorCameraController::OnUpdate(Entity cameraEntity, Timestep ts)
{
    if (!cameraEntity || !cameraEntity.HasComponent<TransformComponent>() ||
        !cameraEntity.HasComponent<CameraComponent>())
    {
        return;
    }

    auto& tc = cameraEntity.GetComponent<TransformComponent>();
    float deltaTime = ts;

    // Viewport dimensions for calculations
    m_Camera.SetViewportSize((uint32_t)EditorLayer::Get().GetViewportSize().x, (uint32_t)EditorLayer::Get().GetViewportSize().y);

    // Load settings from project
    float moveSpeed = m_MoveSpeed;
    float boostMultiplier = m_BoostMultiplier;
    float sensitivity = 1.0f;

    if (auto project = Project::GetActive())
    {
        const auto& editorSettings = project->GetConfig().Editor;
        moveSpeed = editorSettings.CameraMoveSpeed;
        boostMultiplier = editorSettings.CameraBoostMultiplier;
        sensitivity = editorSettings.CameraRotationSpeed;
    }

    // 1. Sync from current transform if it was changed externally (e.g. Inspector)
    if (fabsf(tc.Rotation.x - m_Camera.GetPitch() * RAD2DEG) > 0.01f || fabsf(tc.Rotation.y - m_Camera.GetYaw() * RAD2DEG) > 0.01f)
    {
        m_Camera.SetPitch(tc.Rotation.x * DEG2RAD);
        m_Camera.SetYaw(tc.Rotation.y * DEG2RAD);
    }
    m_Camera.SetFocalPoint(Vector3Add(tc.Translation, Vector3Scale(m_Camera.GetForwardDirection(), m_Camera.GetDistance())));

    const Vector2& mouse = Input::GetMousePosition();
    Vector2 delta = Input::GetMouseDelta();
    m_InitialMousePosition = mouse;

    if (Input::IsKeyDown(KEY_LEFT_ALT))
    {
        if (Input::IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
        }
        else if (Input::IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            m_Camera.MousePan(delta);
        }
        else if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            m_Camera.MouseZoom(delta.y);
        }
    }
    else if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        // Fly mode (FPS style)
        m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});

        float speed = moveSpeed * deltaTime;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT))
        {
            speed *= boostMultiplier;
        }

        Vector3 forward = m_Camera.GetForwardDirection();
        Vector3 right = m_Camera.GetRightDirection();
        Vector3 up = {0, 1, 0};

        if (Input::IsKeyDown(KEY_W))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(forward, speed));
        }
        if (Input::IsKeyDown(KEY_S))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(forward, speed));
        }
        if (Input::IsKeyDown(KEY_D))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(right, speed));
        }
        if (Input::IsKeyDown(KEY_A))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(right, speed));
        }
        if (Input::IsKeyDown(KEY_E))
        {
            tc.Translation = Vector3Add(tc.Translation, Vector3Scale(up, speed));
        }
        if (Input::IsKeyDown(KEY_Q))
        {
            tc.Translation = Vector3Subtract(tc.Translation, Vector3Scale(up, speed));
        }

        // In fly mode, focal point follows position at fixed distance
        m_Camera.SetFocalPoint(Vector3Add(tc.Translation, Vector3Scale(forward, m_Camera.GetDistance())));
    }

    float wheel = Input::GetMouseWheelMove();
    if (wheel != 0)
    {
        m_Camera.MouseZoom(wheel);
    }

    // Update Transform logic
    tc.RotationQuat = QuaternionFromEuler(m_Camera.GetPitch(), m_Camera.GetYaw(), 0.0f);
    tc.Rotation.x = m_Camera.GetPitch() * RAD2DEG;
    tc.Rotation.y = m_Camera.GetYaw() * RAD2DEG;
    tc.Rotation.z = 0.0f;

    if (!Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        // Only drive position from focal point if NOT in fly mode
        tc.Translation = m_Camera.CalculatePosition();
    }
}

} // namespace CHEngine
