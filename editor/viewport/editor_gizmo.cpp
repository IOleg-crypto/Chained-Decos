#include "editor_gizmo.h"

#include "editor_gui.h"
#include "editor_layer.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/hierarchy_component.h"
#include "undo/modify_component_command.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Chained
{

bool EditorGizmo::RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize,
                                  const Chained::Camera3D& camera)
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
    glm::mat4 modelMat = transform.WorldTransform;

    // 1. Setup ImGuizmo
    ImGuizmo::SetOrthographic(camera.Projection != 0);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList()); // Explicitly use current window draw list

    // Ensure we are using absolute screen coordinates for SetRect
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
    if (camera.Projection == 0) // Perspective
    {
        projection = glm::perspective(glm::radians(camera.FovY), aspect, camera.NearClip, camera.FarClip);
    }
    else // Orthographic
    {
        // Align with SceneRenderer: total height is camera.Fovy (which is OrthographicSize)
        float top = camera.FovY * 0.5f;
        float right = top * aspect;
        projection = glm::ortho(-right, right, -top, top, camera.NearClip, camera.FarClip);
    }

    // ImGuizmo uses a right-handed coordinate system by default, but it's good to be explicit
    // if there was any handedness mismatch.

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
        glm::mat4 localMat = modelMat;
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& hierarchy = entity.GetComponent<HierarchyComponent>();
            if (hierarchy.Parent != entt::null && scene->GetRegistry().valid(hierarchy.Parent) &&
                scene->GetRegistry().all_of<TransformComponent>(hierarchy.Parent))
            {
                const auto& parentTransform = scene->GetRegistry().get<TransformComponent>(hierarchy.Parent);
                localMat = glm::inverse(parentTransform.WorldTransform) * modelMat;
            }
        }

        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMat), glm::value_ptr(translation),
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

} // namespace Chained
