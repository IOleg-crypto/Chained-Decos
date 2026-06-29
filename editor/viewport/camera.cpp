#include "camera.h"
#include "editor/layer.h"
#include "engine/core/input.h"
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/transform_component.h"
#include "engine/project/project.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Chained
{

EditorCameraController::EditorCameraController()
{
    SetPerspective(glm::radians(45.0f), 0.1f, 10000.0f);
    UpdateView();
}

void EditorCameraController::OnUpdate(Entity cameraEntity, Timestep ts, const glm::vec2& viewportSize)
{
    m_ViewportWidth = (uint32_t)viewportSize.x;
    m_ViewportHeight = (uint32_t)viewportSize.y;

    float deltaTime = ts;
    float moveSpeed = m_MoveSpeed;
    float boostMultiplier = m_BoostMultiplier;
    float sensitivity = 1.0f;



    bool hasEntity = cameraEntity && cameraEntity.HasComponent<TransformComponent>() && cameraEntity.HasComponent<CameraComponent>();

    if (hasEntity && !Core::Input::IsMouseButtonDown(MouseCode::ButtonRight) && !Core::Input::IsMouseButtonDown(MouseCode::ButtonMiddle))
    {
        auto& tc = cameraEntity.GetComponent<TransformComponent>();
        if (std::isfinite(tc.Rotation.x) && std::isfinite(tc.Rotation.y))
        {
             if (fabsf(tc.Rotation.x - m_Pitch) > 0.01f || fabsf(tc.Rotation.y - m_Yaw) > 0.01f)
             {
                 m_Pitch = tc.Rotation.x;
                 m_Yaw = tc.Rotation.y;
                 UpdateView();
             }
        }
    }

    glm::vec2 delta = Core::Input::GetMouseDelta();

    if (Core::Input::IsMouseButtonDown(MouseCode::ButtonRight))
    {
        MouseRotate({delta.x * sensitivity, delta.y * sensitivity});

        float speed = moveSpeed * deltaTime;
        if (Core::Input::IsKeyDown(KeyCode::LeftShift)) speed *= boostMultiplier;

        glm::vec3 fwd = GetForwardDirection();
        glm::vec3 rgt = GetRightDirection();
        glm::vec3 upg = {0, 1, 0};

        glm::vec3 currentPos = CalculatePosition();

        if (Core::Input::IsKeyDown(KeyCode::W)) currentPos += fwd * speed;
        if (Core::Input::IsKeyDown(KeyCode::S)) currentPos -= fwd * speed;
        if (Core::Input::IsKeyDown(KeyCode::D)) currentPos += rgt * speed;
        if (Core::Input::IsKeyDown(KeyCode::A)) currentPos -= rgt * speed;
        if (Core::Input::IsKeyDown(KeyCode::E)) currentPos += upg * speed;
        if (Core::Input::IsKeyDown(KeyCode::Q)) currentPos -= upg * speed;

        m_FocalPoint = currentPos + (fwd * m_Distance);
        UpdateView();
    }

    if (Core::Input::IsMouseButtonDown(MouseCode::ButtonMiddle))
    {
        if (Core::Input::IsKeyDown(KeyCode::LeftShift)) MousePan(delta);
        else MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
    }

    if (Core::Input::IsKeyDown(KeyCode::LeftAlt) && Core::Input::IsMouseButtonDown(MouseCode::ButtonLeft))
    {
        MouseRotate({delta.x * sensitivity, delta.y * sensitivity});
    }

    float wheel = Core::Input::GetMouseWheelMove();
    if (wheel != 0) MouseZoom(wheel);

    if (hasEntity)
    {
        auto& tc = cameraEntity.GetComponent<TransformComponent>();
        ComponentUtils::SetRotation(tc, glm::vec3(m_Pitch, m_Yaw, 0.0f));
        if (!Core::Input::IsMouseButtonDown(MouseCode::ButtonRight))
        {
            ComponentUtils::SetTranslation(tc, CalculatePosition());
        }
    }
}

void EditorCameraController::UpdateView()
{
    glm::vec3 position = CalculatePosition();
    glm::quat orientation = GetOrientation();
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(orientation);
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

void EditorCameraController::MousePan(const glm::vec2& delta)
{
    auto [xSpeed, ySpeed] = PanSpeed();
    m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
    m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
    UpdateView();
}

void EditorCameraController::MouseRotate(const glm::vec2& delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * RotationSpeed();
    m_Pitch += delta.y * RotationSpeed();
    UpdateView();
}

void EditorCameraController::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
    if (m_Distance < 0.1f)
    {
        m_FocalPoint += GetForwardDirection();
        m_Distance = 0.1f;
    }
    UpdateView();
}

glm::vec3 EditorCameraController::GetUpDirection() const { return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f)); }
glm::vec3 EditorCameraController::GetRightDirection() const { return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f)); }
glm::vec3 EditorCameraController::GetForwardDirection() const { return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f)); }
glm::vec3 EditorCameraController::CalculatePosition() const { return m_FocalPoint - GetForwardDirection() * m_Distance; }
glm::quat EditorCameraController::GetOrientation() const { return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f)); }

std::pair<float, float> EditorCameraController::PanSpeed() const
{
    float x = std::min((float)m_ViewportWidth / 1000.0f, 2.4f);
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
    float y = std::min((float)m_ViewportHeight / 1000.0f, 2.4f);
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
    return {xFactor, yFactor};
}

float EditorCameraController::RotationSpeed() const { return 0.8f; }

float EditorCameraController::ZoomSpeed() const
{
    float distance = m_Distance * 0.2f;
    distance = std::max(distance, 0.0f);
    float speed = distance * distance;
    speed = std::min(speed, 100.0f);
    return speed;
}

} // namespace Chained
