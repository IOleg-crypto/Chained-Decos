#ifndef CD_EDITOR_VIEWPORT_VIEWPORT_GIZMO_H
#define CD_EDITOR_VIEWPORT_VIEWPORT_GIZMO_H

#include "editor/editor_types.h"
#include <cmath>
#include <entt/entt.hpp>
#include <imgui.h>
#include <memory>
#include <raylib.h>


namespace CHEngine
{
class Scene;
class CommandHistory;

class ViewportGizmo
{
public:
    ViewportGizmo() = default;
    ~ViewportGizmo() = default;

    bool RenderAndHandle(std::shared_ptr<Scene> scene, const Camera3D &camera, entt::entity entity,
                         Tool currentTool, ImVec2 viewportSize, bool isHovered,
                         CommandHistory *history = nullptr);
    bool IsHovered() const
    {
        return m_GizmoHovered;
    }

    bool IsDragging() const
    {
        return m_DraggingAxis != GizmoAxis::NONE;
    }

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

    // Snapping config
    bool m_SnappingEnabled = false;
    float m_GridSize = 1.0f;
    float m_RotationStep = 15.0f; // Degrees

    // Internal state for dragging
    Vector3 m_OriginalPosition = {0, 0, 0};
    Vector3 m_OriginalRotation = {0, 0, 0};
    Vector3 m_OriginalScale = {1, 1, 1};

private:
    static float SnapValue(float value, float step);
};

} // namespace CHEngine
#endif // CD_EDITOR_VIEWPORT_VIEWPORT_GIZMO_H
