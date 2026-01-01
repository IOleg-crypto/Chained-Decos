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

/**
 * @brief Main editor viewport for 3D rendering and interaction
 */
class ViewportPanel
{
public:
    ViewportPanel() = default;
    ~ViewportPanel();

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender(
        SceneState state, SelectionType selectionType,
        const std::shared_ptr<GameScene> &legacyScene, const std::shared_ptr<Scene> &modernScene,
        const Camera3D &camera, int selectedObjectIndex, Tool currentTool,
        const std::function<void(int)> &onSelect, CommandHistory *history,
        const std::function<void(const std::string &, const Vector3 &)> &onAssetDropped = nullptr);

    // --- State & Configuration ---
public:
    bool IsFocused() const;
    bool IsHovered() const;
    bool IsVisible() const;
    void SetVisible(bool visible);

    ImVec2 GetSize() const;

    // --- Coordinate Transformations ---
public:
    Vector2 GetViewportMousePosition() const;
    Vector2 GetViewportWorldToScreen(Vector3 worldPos, Camera3D camera) const;

    // --- Internal Helpers ---
private:
    void Resize(uint32_t width, uint32_t height);

    // --- Member Variables ---
private:
    RenderTexture2D m_ViewportTexture = {0};
    uint32_t m_Width = 0, m_Height = 0;
    bool m_Focused = false, m_Hovered = false;
    bool m_isVisible = true;

    // Grid
    EditorGrid m_Grid;
    bool m_GridInitialized = false;

    // Sub-systems
    ViewportGizmo m_Gizmo;
    ViewportRenderer m_Renderer;
};
} // namespace CHEngine

#endif // VIEWPANEL_H
