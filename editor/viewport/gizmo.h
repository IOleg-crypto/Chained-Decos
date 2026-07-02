#ifndef CH_EDITOR_GIZMO_H
#define CH_EDITOR_GIZMO_H

#include "engine/scene/scene.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "ImGuizmo.h"

namespace Chained
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
    bool RenderAndHandle(GizmoType type, ImVec2 viewportPos, ImVec2 viewportSize, const Chained::Camera3D& camera);

    bool IsHovered() const { return ImGuizmo::IsOver(); }
    bool IsDragging() const { return ImGuizmo::IsUsing(); }

    // Snapping
    void SetSnapping(bool enabled) { m_SnappingEnabled = enabled; }
    bool IsSnappingEnabled() const { return m_SnappingEnabled; }

    void SetGridSize(float size)      { m_TranslationSnap = size; }
    void SetRotationStep(float step)  { m_RotationSnap = step; }
    void SetScaleStep(float step)     { m_ScaleSnap = step; }

    float GetGridSize() const     { return m_TranslationSnap; }
    float GetRotationStep() const { return m_RotationSnap; }
    float GetScaleStep() const    { return m_ScaleSnap; }

    void SetLocalSpace(bool local) { m_IsLocalSpace = local; }
    bool IsLocalSpace() const      { return m_IsLocalSpace; }

private:
    bool m_SnappingEnabled = false;
    
    
    float m_TranslationSnap = 1.0f;
    float m_RotationSnap = 45.0f;
    float m_ScaleSnap = 0.1f;
    
    bool m_IsLocalSpace = false;

    // Undo state
    TransformComponent m_OldTransform;
    bool m_WasUsing = false;
};

} // namespace Chained

#endif // CH_EDITOR_GIZMO_H