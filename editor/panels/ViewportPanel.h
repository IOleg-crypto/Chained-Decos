#ifndef VIEWPANEL_H
#define VIEWPANEL_H

#include "editor/EditorTypes.h"
#include "editor/logic/undo/CommandHistory.h"
#include "editor/utils/EditorGrid.h"
#include "editor/viewport/ViewportGizmo.h"
#include "editor/viewport/ViewportRenderer.h"
#include "scene/camera/CameraController.h"
#include "scene/resources/map/GameScene.h"
#include <cstdint>
#include <functional>
#include <imgui.h>
#include <memory>
#include <raylib.h>

namespace CHEngine
{
class Scene;
class CommandHistory;

class ViewportPanel
{
public:
    ViewportPanel() = default;
    ~ViewportPanel();

    void OnImGuiRender(
        SceneState state, const std::shared_ptr<GameScene> &legacyScene,
        const std::shared_ptr<Scene> &modernScene, const Camera3D &camera, int selectedObjectIndex,
        Tool currentTool, const std::function<void(int)> &onSelect, CommandHistory *history,
        const std::function<void(const std::string &, const Vector3 &)> &onAssetDropped = nullptr);

    bool IsFocused() const
    {
        return m_Focused;
    }
    bool IsHovered() const
    {
        return m_Hovered;
    }

    Vector2 GetViewportMousePosition() const;
    Vector2 GetViewportWorldToScreen(Vector3 worldPos, Camera3D camera) const;
    ImVec2 GetSize() const
    {
        return {(float)m_Width, (float)m_Height};
    }

    bool IsVisible() const
    {
        return m_isVisible;
    }
    void SetVisible(bool visible)
    {
        m_isVisible = visible;
    }

private:
    void Resize(uint32_t width, uint32_t height);

private:
    RenderTexture2D m_ViewportTexture = {0};
    uint32_t m_Width = 0, m_Height = 0;
    bool m_Focused = false, m_Hovered = false;

    // Grid
    EditorGrid m_Grid;
    bool m_GridInitialized = false;

    // Extracted components
    ViewportGizmo m_Gizmo;
    ViewportRenderer m_Renderer;
    bool m_isVisible = true;
};
} // namespace CHEngine

#endif // VIEWPANEL_H
