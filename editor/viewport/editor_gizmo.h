#ifndef CH_EDITOR_GIZMO_H
#define CH_EDITOR_GIZMO_H

#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include "raylib.h"
#include <imgui.h>

namespace CH
{

enum class GizmoAxis
{
    NONE,
    X,
    Y,
    Z,
    XY,
    YZ,
    XZ
};

enum class GizmoType
{
    SELECT,
    TRANSLATE,
    ROTATE,
    SCALE
};

class EditorGizmo
{
public:
    EditorGizmo() = default;
    ~EditorGizmo() = default;

    bool RenderAndHandle(Scene *scene, const Camera3D &camera, Entity entity, GizmoType type,
                         ImVec2 viewportSize, bool isHovered);

    bool IsHovered() const
    {
        return m_GizmoHovered;
    }

    bool IsDragging() const
    {
        return m_DraggingAxis != GizmoAxis::NONE;
    }

    // Snapping configuration
    void SetSnapping(bool enabled)
    {
        m_SnappingEnabled = enabled;
    }
    void SetGridSize(float size)
    {
        m_GridSize = size;
    }
    void SetRotationStep(float step)
    {
        m_RotationStep = step;
    }

    bool IsSnappingEnabled() const
    {
        return m_SnappingEnabled;
    }
    float GetGridSize() const
    {
        return m_GridSize;
    }
    float GetRotationStep() const
    {
        return m_RotationStep;
    }

private:
    // Gizmo state
    GizmoAxis m_DraggingAxis = GizmoAxis::NONE;
    bool m_GizmoHovered = false;
    ImVec2 m_InitialMousePos = {0, 0};
    Vector3 m_InitialObjectValue = {0, 0, 0};
    TransformComponent m_OldTransform;

    // Snapping config
    bool m_SnappingEnabled = false;
    float m_GridSize = 1.0f;
    float m_RotationStep = 15.0f; // Degrees

    static float SnapValue(float value, float step);
};

} // namespace CH

#endif // CH_EDITOR_GIZMO_H
