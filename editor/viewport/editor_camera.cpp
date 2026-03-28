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
    if (fabsf(tc.Rotation.x - m_Camera.GetPitch() * glm::degrees(1.0f)) > 0.01f || fabsf(tc.Rotation.y - m_Camera.GetYaw() * glm::degrees(1.0f)) > 0.01f)
    {
        m_Camera.SetPitch(tc.Rotation.x * glm::radians(1.0f));
        m_Camera.SetYaw(tc.Rotation.y * glm::radians(1.0f));
    }
    Vector3 tcTranslation = *reinterpret_cast<const Vector3*>(&tc.Translation);
    glm::vec3 translation = *reinterpret_cast<const glm::vec3*>(&tcTranslation);
    m_Camera.SetFocalPoint(translation + (m_Camera.GetForwardDirection() * m_Camera.GetDistance()));

    const Vector2& mouse = Input::GetMousePosition();
    Vector2 delta = Input::GetMouseDelta();
    m_InitialMousePosition = mouse;

    if (Input::IsKeyDown(Key::LeftAlt))
    {
        if (Input::IsMouseButtonDown(Mouse::ButtonLeft))
        {
            m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
        }
        else if (Input::IsMouseButtonDown(Mouse::ButtonMiddle))
        {
            m_Camera.MousePan(delta);
        }
        else if (Input::IsMouseButtonDown(Mouse::ButtonRight))
        {
            m_Camera.MouseZoom(delta.y);
        }
    }
    else if (Input::IsMouseButtonDown(Mouse::ButtonRight))
    {
        // Fly mode (FPS style)
        m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});

        float speed = moveSpeed * deltaTime;
        if (Input::IsKeyDown(Key::LeftShift))
        {
            speed *= boostMultiplier;
        }

        Vector3 forward = m_Camera.GetForwardDirection();
        Vector3 right = m_Camera.GetRightDirection();
        Vector3 up = {0, 1, 0};
        
        glm::vec3 fwd(forward.x, forward.y, forward.z);
        glm::vec3 rgt(right.x, right.y, right.z);
        glm::vec3 upg(up.x, up.y, up.z);

        glm::vec3 tcPos = tc.Translation;
        if (Input::IsKeyDown(Key::W))
        {
            tcPos = tcPos + (fwd * speed);
        }
        if (Input::IsKeyDown(Key::S))
        {
            tcPos = tcPos - (fwd * speed);
        }
        if (Input::IsKeyDown(Key::D))
        {
            tcPos = tcPos + (rgt * speed);
        }
        if (Input::IsKeyDown(Key::A))
        {
            tcPos = tcPos - (rgt * speed);
        }
        if (Input::IsKeyDown(Key::E))
        {
            tcPos = tcPos + (upg * speed);
        }
        if (Input::IsKeyDown(Key::Q))
        {
            tcPos = tcPos - (upg * speed);
        }
        tc.Translation = tcPos;

        // In fly mode, focal point follows position at fixed distance
        m_Camera.SetFocalPoint(tcPos + (fwd * m_Camera.GetDistance()));
    }

    float wheel = Input::GetMouseWheelMove();
    if (wheel != 0)
    {
        m_Camera.MouseZoom(wheel);
    }

    // Update Transform logic
    glm::quat q = glm::quat(glm::vec3(m_Camera.GetPitch(), m_Camera.GetYaw(), 0.0f));
    tc.RotationQuat = q;
    tc.Rotation.x = m_Camera.GetPitch() * glm::degrees(1.0f);
    tc.Rotation.y = m_Camera.GetYaw() * glm::degrees(1.0f);
    tc.Rotation.z = 0.0f;

    if (!Input::IsMouseButtonDown(Mouse::ButtonRight))
    {
        // Only drive position from focal point if NOT in fly mode
        Vector3 pos = m_Camera.CalculatePosition();
        tc.Translation = *reinterpret_cast<const glm::vec3*>(&pos);
    }
}

} // namespace CHEngine
