#include "editor_gizmo.h"
#include "editor/actions/editor_actions.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "raymath.h"
#include "editor_gui.h"
#include "undo/modify_component_command.h"


namespace CHEngine
{

bool EditorGizmo::RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize, const Camera3D& camera)
{
    auto &layer = EditorLayer::Get();
    Scene *scene = layer.GetActiveScene().get();
    Entity entity = layer.GetSelectedEntity();

    if (!scene || !entity || !entity.HasComponent<TransformComponent>() || type == GizmoType::NONE || layer.GetSceneState() == SceneState::Play)
    {
        // Trace why gizmo is skipped
        if (type != GizmoType::NONE && entity)
        {
             static int skipCount = 0;
             if (skipCount++ % 60 == 0)
                 CH_CORE_TRACE("EditorGizmo: Skip. Scene={}, Entity={}, HasTransform={}, Tool={}, IsPlay={}", 
                    (bool)scene, (uint32_t)entity, entity.HasComponent<TransformComponent>(), (int)type, layer.GetSceneState() == SceneState::Play);
        }
        return false;
    }

    auto &transform = entity.GetComponent<TransformComponent>();

    // 1. Setup ImGuizmo
    ImGuizmo::SetOrthographic(camera.projection == CAMERA_ORTHOGRAPHIC);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    // 2. Prepare View/Projection matrices (Transposed for ImGuizmo/OpenGL column-major)
    Matrix view = MatrixTranspose(GetCameraMatrix(camera));
    Matrix projection;
    if (camera.projection == CAMERA_PERSPECTIVE)
    {
        projection = MatrixPerspective(
            camera.fovy * DEG2RAD, viewportSize.x / viewportSize.y, 0.01f, 1000.0f);
    }
    else
    {
        float aspect = viewportSize.x / viewportSize.y;
        float right = camera.fovy * aspect * 0.5f;
        float left = -right;
        float top = camera.fovy * 0.5f;
        float bottom = -top;
        projection = MatrixOrtho(left, right, bottom, top, 0.01f, 1000.0f);
    }

    projection = MatrixTranspose(projection);

    // 3. Prepare Model matrix (Transposed for ImGuizmo)
    Matrix model = MatrixTranspose(transform.GetTransform());

    // 4. Handle Snapping
    float *snap = m_SnappingEnabled ? m_SnapValues : nullptr;

    // 5. Manipulation
    ImGuizmo::MODE mode = m_IsLocalSpace ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetID(0);

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

    if (ImGuizmo::IsUsing())
    {
        // Matrix finalModel = MatrixTranspose(model); // model is already modified and in correct space
        Vector3 translation, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents((float *)&model, (float *)&translation,
                                              (float *)&rotation, (float *)&scale);

        transform.Translation = translation;
        transform.Rotation.x = rotation.x * DEG2RAD;
        transform.Rotation.y = rotation.y * DEG2RAD;
        transform.Rotation.z = rotation.z * DEG2RAD;
        transform.RotationQuat =
            QuaternionFromEuler(transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
        transform.Scale = scale;
    }
    else if (m_WasUsing)
    {
        m_WasUsing = false;
        EditorActions::PushCommand(std::make_unique<ModifyComponentCommand<TransformComponent>>(
            entity, m_OldTransform, transform, "Transform Entity"));
    }

    bool hovered = ImGuizmo::IsOver();
    bool usingGizmo = ImGuizmo::IsUsing();

    if (ImGui::IsMouseClicked(0))
    {
        CH_CORE_WARN("Gizmo Click: Over={}, Using={}, Pos({},{}), Rect({},{},{},{})", 
            hovered, usingGizmo, ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y,
            viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
    }

    return hovered || usingGizmo;
}

} // namespace CHEngine
