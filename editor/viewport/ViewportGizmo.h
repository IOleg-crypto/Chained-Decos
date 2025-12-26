#pragma once

#include "editor/EditorTypes.h"
#include "scene/camera/CameraController.h"
#include "scene/resources/map/GameScene.h"
#include <imgui.h>
#include <memory>
#include <raylib.h>


namespace CHEngine
{

/**
 * @brief Hazel-style viewport gizmo system
 * Handles 3D gizmo rendering and interaction for object transformation
 */
class ViewportGizmo
{
public:
    ViewportGizmo() = default;
    ~ViewportGizmo() = default;

    /**
     * @brief Render and handle gizmo interaction
     * @param scene Scene containing objects
     * @param camera Current camera
     * @param selectedObjectIndex Index of selected object
     * @param currentTool Current transformation tool (Move/Rotate/Scale)
     * @param viewportSize Viewport dimensions
     * @param isHovered Whether viewport is hovered
     * @return True if gizmo is being interacted with
     */
    bool RenderAndHandle(const std::shared_ptr<GameScene> &scene, const Camera3D &camera,
                         int selectedObjectIndex, Tool currentTool, ImVec2 viewportSize,
                         bool isHovered);

    /**
     * @brief Check if gizmo is currently hovered
     */
    bool IsHovered() const
    {
        return m_GizmoHovered;
    }

    /**
     * @brief Check if currently dragging a gizmo axis
     */
    bool IsDragging() const
    {
        return m_DraggingAxis != GizmoAxis::NONE;
    }

private:
    // Gizmo state
    GizmoAxis m_DraggingAxis = GizmoAxis::NONE;
    bool m_GizmoHovered = false;
    ImVec2 m_InitialMousePos = {0, 0};
    Vector3 m_InitialObjectValue = {0, 0, 0};

    // Helper methods
    void DrawGizmo(const MapObjectData &obj, const Camera3D &camera, Tool tool);
    void HandleGizmoDrag(MapObjectData &obj, Tool tool, ImVec2 currentMouse,
                         const Camera3D &camera);
    bool CheckGizmoHover(const Vector3 &handlePos, const Camera3D &camera, ImVec2 viewportSize);
};

} // namespace CHEngine
