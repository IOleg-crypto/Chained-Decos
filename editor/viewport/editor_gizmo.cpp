#include "editor_gizmo.h"
#include "editor/actions/editor_actions.h"
#include "editor_gui.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "undo/modify_component_command.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
{

bool EditorGizmo::RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize, const CHEngine::Camera3D& camera)
{
    auto& layer = EditorLayer::Get();
    Scene* scene = layer.GetActiveScene().get();
    Entity entity = layer.GetSelectedEntity();

    if (!scene || !entity || !entity.HasComponent<TransformComponent>() || type == GizmoType::NONE ||
        layer.GetSceneState() == SceneState::Play)
    {
        return false;
    }

    auto& transform = entity.GetComponent<TransformComponent>();

    // 1. Setup ImGuizmo
    ImGuizmo::SetOrthographic(camera.Projection == 1); // Assuming 1 is Orthographic
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    // 2. Prepare View/Projection matrices
    glm::mat4 view = glm::lookAt(camera.Position, camera.Target, camera.Up);
    glm::mat4 projection;
    
    float aspect = viewportSize.x / viewportSize.y;
    if (camera.Projection == 0) // Perspective
    {
        projection = glm::perspective(glm::radians(camera.Fovy), aspect, 0.01f, 1000.0f);
    }
    else // Orthographic
    {
        float top = camera.Fovy * 0.5f;
        float right = top * aspect;
        projection = glm::ortho(-right, right, -top, top, 0.01f, 1000.0f);
    }

    // 3. Prepare Model matrix
    glm::mat4 modelMat = transform.GetTransform();

    // 4. Handle Snapping
    float* snap = m_SnappingEnabled ? m_SnapValues : nullptr;

    // 5. Manipulation
    ImGuizmo::MODE mode = m_IsLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    
    if (ImGuizmo::IsUsing())
    {
        if (!m_WasUsing)
        {
            m_WasUsing = true;
            m_OldTransform = transform;
        }
    }

    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), (ImGuizmo::OPERATION)type, mode, glm::value_ptr(modelMat), NULL, snap);

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMat), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));

        transform.Translation = translation;
        transform.Rotation = rotation; // ImGuizmo returns degrees
        transform.SetRotation(glm::radians(rotation));
        transform.Scale = scale;
        transform.IsDirty = true;
    }
    else if (m_WasUsing)
    {
        m_WasUsing = false;
        EditorActions::PushCommand(std::make_unique<ModifyComponentCommand<TransformComponent>>(
            entity, m_OldTransform, transform, "Transform Entity"));
    }

    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

} // namespace CHEngine
