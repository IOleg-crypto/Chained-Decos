#ifndef CD_EDITOR_PANELS_VIEWPORT_PANEL_H
#define CD_EDITOR_PANELS_VIEWPORT_PANEL_H

#include "editor/editor_types.h"
#include "editor/logic/undo/command_history.h"
#include "editor/utils/editor_grid.h"
#include "editor/viewport/viewport_gizmo.h"
#include "editor/viewport/viewport_renderer.h"
#include "editor_panel.h"
#include <cstdint>
#include <memory>
#include <raylib.h>

namespace CHEngine
{
class Scene;
class CommandHistory;
class EditorSceneActions;
class EditorProjectActions;
class SelectionManager;
class SceneSimulationManager;
class EditorCamera;
class EditorEntityFactory;

/**
 * @brief Main editor viewport for 3D rendering and interaction
 */
class ViewportPanel : public EditorPanel
{
public:
    ViewportPanel();
    virtual ~ViewportPanel();

    virtual void OnImGuiRender() override;

    bool IsFocused() const
    {
        return m_Focused;
    }
    bool IsHovered() const
    {
        return m_Hovered;
    }
    ImVec2 GetSize() const
    {
        return {(float)m_Width, (float)m_Height};
    }

    // --- Coordinate Transformations ---
public:
    Vector2 GetViewportMousePosition() const;
    Vector2 GetViewportWorldToScreen(Vector3 worldPos, Camera3D camera) const;

    // --- Internal Helpers ---
private:
    void Resize(uint32_t width, uint32_t height);

    // --- Member Variables ---
private:
    EditorSceneActions *m_SceneActions = nullptr;
    EditorProjectActions *m_ProjectActions = nullptr;
    SelectionManager *m_SelectionManager = nullptr;
    SceneSimulationManager *m_SimulationManager = nullptr;
    EditorCamera *m_Camera = nullptr;
    EditorEntityFactory *m_EntityFactory = nullptr;
    CommandHistory *m_CommandHistory = nullptr;
    Tool *m_ActiveTool = nullptr;

    RenderTexture2D m_ViewportTexture = {0};
    uint32_t m_Width = 0, m_Height = 0;
    bool m_Focused = false, m_Hovered = false;

    // Grid
    EditorGrid m_Grid;
    bool m_GridInitialized = false;

    // Sub-systems
    ViewportGizmo m_Gizmo;
    ViewportRenderer m_Renderer;

    bool m_ShowHUD = false;
    bool m_ShowPhysicsDebug = false;
    bool m_ShowFPS = false;
};
} // namespace CHEngine

#endif // CD_EDITOR_PANELS_VIEWPORT_PANEL_H
