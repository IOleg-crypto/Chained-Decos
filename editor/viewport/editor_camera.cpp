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
    float deltaTime = ts;

    // Viewport dimensions for calculations
    m_Camera.SetViewportSize((uint32_t)EditorLayer::Get().GetViewportSize().x,
                             (uint32_t)EditorLayer::Get().GetViewportSize().y);

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

    bool hasEntity =
        cameraEntity && cameraEntity.HasComponent<TransformComponent>() && cameraEntity.HasComponent<CameraComponent>();

    // 1. Sync from entity transform if changed externally (e.g. Inspector)
    if (hasEntity)
    {
        auto& tc = cameraEntity.GetComponent<TransformComponent>();
        if (fabsf(tc.Rotation.x - m_Camera.GetPitch()) > 0.01f || fabsf(tc.Rotation.y - m_Camera.GetYaw()) > 0.01f)
        {
            m_Camera.SetPitch(tc.Rotation.x);
            m_Camera.SetYaw(tc.Rotation.y);
        }
        glm::vec3 tcTranslation = *reinterpret_cast<const glm::vec3*>(&tc.Translation);
        m_Camera.SetFocalPoint(tcTranslation + (m_Camera.GetForwardDirection() * m_Camera.GetDistance()));
    }

    const glm::vec2& mouse = Input::GetMousePosition();
    glm::vec2 delta = Input::GetMouseDelta();
    m_InitialMousePosition = mouse;

    // === Right Mouse Button: Rotate + Fly (FPS-style) ===
    if (Input::IsMouseButtonDown(Mouse::ButtonRight))
    {
        m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});

        float speed = moveSpeed * deltaTime;
        if (Input::IsKeyDown(Key::LeftShift))
        {
            speed *= boostMultiplier;
        }

        glm::vec3 fwd = m_Camera.GetForwardDirection();
        glm::vec3 rgt = m_Camera.GetRightDirection();
        glm::vec3 upg = {0, 1, 0};

        glm::vec3 currentPos =
            hasEntity ? cameraEntity.GetComponent<TransformComponent>().Translation : m_Camera.CalculatePosition();

        if (Input::IsKeyDown(Key::W)) currentPos += fwd * speed;
        if (Input::IsKeyDown(Key::S)) currentPos -= fwd * speed;
        if (Input::IsKeyDown(Key::D)) currentPos += rgt * speed;
        if (Input::IsKeyDown(Key::A)) currentPos -= rgt * speed;
        if (Input::IsKeyDown(Key::E)) currentPos += upg * speed;
        if (Input::IsKeyDown(Key::Q)) currentPos -= upg * speed;

        if (hasEntity)
        {
            cameraEntity.GetComponent<TransformComponent>().SetTranslation(currentPos);
        }

        m_Camera.SetFocalPoint(currentPos + (fwd * m_Camera.GetDistance()));
    }

    // === Middle Mouse: Orbit or Alt+Middle Pan ===
    if (Input::IsMouseButtonDown(Mouse::ButtonMiddle))
    {
        if (Input::IsKeyDown(Key::LeftShift))
        {
            m_Camera.MousePan(delta);
        }
        else
        {
            m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
        }
    }

    // === Alt + Left Mouse: Classic orbit (Maya/Unity style) ===
    if (Input::IsKeyDown(Key::LeftAlt) && Input::IsMouseButtonDown(Mouse::ButtonLeft))
    {
        m_Camera.MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
    }

    // === Scroll wheel: Zoom ===
    float wheel = Input::GetMouseWheelMove();
    if (wheel != 0)
    {
        m_Camera.MouseZoom(wheel);
    }

    // 2. Sync camera state back to entity transform
    if (hasEntity)
    {
        auto& tc = cameraEntity.GetComponent<TransformComponent>();
        tc.SetRotation(glm::vec3(m_Camera.GetPitch(), m_Camera.GetYaw(), 0.0f));

        if (!Input::IsMouseButtonDown(Mouse::ButtonRight))
        {
            glm::vec3 pos = m_Camera.CalculatePosition();
            tc.SetTranslation(pos);
        }
    }
}

} // namespace CHEngine
