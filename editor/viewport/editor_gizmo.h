#ifndef CH_EDITOR_GIZMO_H
#define CH_EDITOR_GIZMO_H

#include "engine/scene/scene.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "raylib.h"

namespace CHEngine
{

enum class GizmoType
{
    NONE = -1,
    TRANSLATE = ImGuizmo::OPERATION::TRANSLATE,
    ROTATE = ImGuizmo::OPERATION::ROTATE,
    SCALE = ImGuizmo::OPERATION::SCALE,
    BOUNDS = ImGuizmo::OPERATION::BOUNDS
};

class EditorGizmo
{
public:
    EditorGizmo() = default;
    ~EditorGizmo() = default;

    // Render and handle gizmo interaction
    // true if the gizmo is being used (captured mouse)
    bool RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize);

    bool IsHovered() const
    {
        return ImGuizmo::IsOver();
    }
    bool IsDragging() const
    {
        return ImGuizmo::IsUsing();
    }

    // Snapping
    void SetSnapping(bool enabled)
    {
        m_SnappingEnabled = enabled;
    }
    void SetGridSize(float size)
    {
        m_SnapValues[0] = m_SnapValues[1] = m_SnapValues[2] = size;
    }
    void SetRotationStep(float step)
    {
        m_SnapValues[0] = m_SnapValues[1] = m_SnapValues[2] = step;
    }

    bool IsSnappingEnabled() const
    {
        return m_SnappingEnabled;
    }
    float GetGridSize() const
    {
        return m_SnapValues[0];
    }

    void SetLocalSpace(bool local)
    {
        m_IsLocalSpace = local;
    }
    bool IsLocalSpace() const
    {
        return m_IsLocalSpace;
    }

private:
    // Snapping
    bool m_SnappingEnabled = false;
    float m_SnapValues[3] = {1.0f, 1.0f, 1.0f};
    bool m_IsLocalSpace = false;

    // Undo state
    TransformComponent m_OldTransform;
    bool m_WasUsing = false;
};

} // namespace CHEngine

#endif // CH_EDITOR_GIZMO_H
