#include "editor_gizmo.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "undo/transform_command.h"
#include <raymath.h>

namespace CHEngine
{

bool EditorGizmo::RenderAndHandle(Scene *scene, const Camera3D &camera, Entity entity,
                                  GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize)
{
    if (!scene || !entity || !entity.HasComponent<TransformComponent>() || type == GizmoType::NONE)
        return false;

    auto &transform = entity.GetComponent<TransformComponent>();

    // 1. Setup ImGuizmo
    ImGuizmo::SetOrthographic(camera.projection == CAMERA_ORTHOGRAPHIC);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    // 2. Prepare View/Projection matrices
    // Raylib is row-major in memory, ImGuizmo is column-major.
    // Transpose them for ImGuizmo.
    Matrix view = MatrixTranspose(GetCameraMatrix(camera));
    Matrix projection = MatrixTranspose(
        MatrixPerspective(camera.fovy * DEG2RAD, viewportSize.x / viewportSize.y, 0.01f, 1000.0f));

    // 3. Prepare Model matrix
    Matrix model = MatrixTranspose(transform.GetTransform());

    // 4. Handle Snapping
    float *snap = m_SnappingEnabled ? m_SnapValues : nullptr;

    // 5. Manipulation
    ImGuizmo::MODE mode = m_IsLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetID(0);

    // Check for Start of Drag (Undo)
    if (ImGuizmo::IsUsing())
    {
        if (!m_WasUsing)
        {
            m_WasUsing = true;
            m_OldTransform = transform;
        }
    }

    ImGuizmo::Manipulate((float *)&view, (float *)&projection, (ImGuizmo::OPERATION)type, mode,
                         (float *)&model, NULL, snap);

    // 6. Apply back to transform if used
    if (ImGuizmo::IsUsing())
    {
        Vector3 translation, rotation, scale;
        // ImGuizmo expects column-major input, and we just manipulated a column-major 'model'
        ImGuizmo::DecomposeMatrixToComponents((float *)&model, (float *)&translation,
                                              (float *)&rotation, (float *)&scale);

        transform.Translation = translation;
        // ImGuizmo returns rotation in DEGREES, TransformComponent uses RADIANS
        transform.Rotation.x = rotation.x * DEG2RAD;
        transform.Rotation.y = rotation.y * DEG2RAD;
        transform.Rotation.z = rotation.z * DEG2RAD;

        // Update Quaternion for internal precision and smooth interpolation
        transform.RotationQuat =
            QuaternionFromEuler(transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);

        transform.Scale = scale;
    }
    else if (m_WasUsing)
    {
        // End of drag - Push Undo
        m_WasUsing = false;
        EditorLayer::GetCommandHistory().PushCommand(
            std::make_unique<TransformCommand>(entity, m_OldTransform, transform));
    }

    return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
}

} // namespace CHEngine
