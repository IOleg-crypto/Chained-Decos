#include "editor_gizmo.h"

#include "editor_gui.h"
#include "editor_layer.h"
#include "engine/scene/components/component_utils.h"
#include "undo/modify_component_command.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{

bool EditorGizmo::RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize,
                                  const CHEngine::Camera3D& camera)
{
    auto& layer = EditorLayer::Get();
    Scene* scene = layer.GetActiveScene().get();
    Entity entity = layer.GetSelectedEntity();

    if (!scene || !entity || !entity.HasComponent<TransformComponent>() || type == GizmoType::NONE ||
        layer.GetSceneState() == SceneState::Play)
    {
        return false;
    }

    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f)
    {
        return false;
    }

    auto& transform = entity.GetComponent<TransformComponent>();

    // 1. Setup ImGuizmo
    ImGuizmo::SetOrthographic(camera.Projection != 0);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    // 2. Prepare View/Projection matrices
    glm::vec3 up = camera.Up;
    if (glm::dot(up, up) <= 0.000001f)
    {
        up = {0.0f, 1.0f, 0.0f};
    }

    glm::vec3 forward = camera.Target - camera.Position;
    if (glm::dot(forward, forward) <= 0.000001f)
    {
        forward = {0.0f, 0.0f, -1.0f};
    }
    else
    {
        forward = glm::normalize(forward);
    }

    glm::mat4 view = glm::lookAt(camera.Position, camera.Position + forward, up);
    glm::mat4 projection;

    const float aspect = viewportSize.x / viewportSize.y;
    constexpr float kNearClip = 0.01f;
    constexpr float kFarClip = 100000.0f;
    if (camera.Projection == 0) // Perspective
    {
        projection = glm::perspective(glm::radians(camera.Fovy), aspect, kNearClip, kFarClip);
    }
    else // Orthographic
    {
        float top = camera.Fovy * 0.5f;
        float right = top * aspect;
        projection = glm::ortho(-right, right, -top, top, kNearClip, kFarClip);
    }

    // 3. Prepare Model matrix
    glm::mat4 modelMat = ComponentUtils::GetTransform(transform);

    // ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(projection), glm::value_ptr(modelMat), m_SnapValues[0]);
    //  4. Handle Snapping
    float* snap = m_SnappingEnabled ? m_SnapValues : nullptr;

    // 5. Manipulation
    ImGuizmo::MODE mode = m_IsLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    const bool wasUsing = m_WasUsing;
    const bool manipulated =
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), static_cast<ImGuizmo::OPERATION>(type),
                             mode, glm::value_ptr(modelMat), nullptr, snap);

    const bool isUsingNow = ImGuizmo::IsUsing();
    if (isUsingNow && !wasUsing)
    {
        m_WasUsing = true;
        m_OldTransform = transform;
    }

    if (manipulated || isUsingNow)
    {
        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMat), glm::value_ptr(translation),
                                              glm::value_ptr(rotation), glm::value_ptr(scale));

        ComponentUtils::SetTranslation(transform, translation);
        ComponentUtils::SetRotation(transform, glm::radians(rotation));
        ComponentUtils::SetScale(transform, scale);
    }
    else if (m_WasUsing && !isUsingNow)
    {
        m_WasUsing = false;

        const bool changed = glm::length(transform.Translation - m_OldTransform.Translation) > 0.0001f ||
                             glm::length(transform.Rotation - m_OldTransform.Rotation) > 0.0001f ||
                             glm::length(transform.Scale - m_OldTransform.Scale) > 0.0001f;

        if (changed)
        {
            EditorLayer::Get().GetCommandHistory().PushCommand(
                std::make_unique<ModifyComponentCommand<TransformComponent>>(entity, m_OldTransform, transform,
                                                                             "Transform Entity"));
        }
    }

    return ImGuizmo::IsOver() || isUsingNow;
}

} // namespace CHEngine
