#pragma once

#include "editor/EditorTypes.h"
#include "scene/camera/core/CameraController.h"
#include "scene/resources/map/core/SceneLoader.h"
#include <imgui.h>
#include <memory>
#include <raylib.h>

namespace CHEngine
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

class EditorLayer; // Forward declaration

class ViewportPanel
{
public:
    ViewportPanel() = default;
    ~ViewportPanel();

    void OnImGuiRender(const std::shared_ptr<GameScene> &scene,
                       const std::shared_ptr<CameraController> &cameraController,
                       EditorLayer *layer);

    bool IsFocused() const
    {
        return m_Focused;
    }
    bool IsHovered() const
    {
        return m_Hovered;
    }

    Vector2 GetViewportMousePosition() const;
    ImVec2 GetSize() const
    {
        return {(float)m_Width, (float)m_Height};
    }

private:
    void Resize(uint32_t width, uint32_t height);

private:
    RenderTexture2D m_ViewportTexture = {0};
    uint32_t m_Width = 0, m_Height = 0;
    bool m_Focused = false, m_Hovered = false;

    // Gizmo state
    GizmoAxis m_DraggingAxis = GizmoAxis::NONE;
    Vector3 m_InitialObjectValue = {0, 0, 0};
    Vector2 m_InitialMousePos = {0, 0};
};
} // namespace CHEngine
