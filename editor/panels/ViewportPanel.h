#ifndef VIEWPANEL_H
#define VIEWPANEL_H

#include "EditorPanel.h"
#include "editor/EditorTypes.h"
#include "editor/logic/undo/CommandHistory.h"
#include "editor/utils/EditorGrid.h"
#include "editor/viewport/ViewportGizmo.h"
#include "editor/viewport/ViewportRenderer.h"
#include <cstdint>
#include <memory>
#include <raylib.h>

namespace CHEngine
{
class Scene;
class CommandHistory;
class EditorSceneActions;
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
    ViewportPanel(EditorSceneActions *sceneActions, SelectionManager *selection,
                  SceneSimulationManager *simulation, EditorCamera *camera,
                  EditorEntityFactory *factory, CommandHistory *history);
    virtual ~ViewportPanel();

    virtual void OnImGuiRender() override;

    // --- State & Configuration ---
public:
    bool IsFocused() const;
    bool IsHovered() const;

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
    EditorSceneActions *m_SceneActions = nullptr;
    SelectionManager *m_SelectionManager = nullptr;
    SceneSimulationManager *m_SimulationManager = nullptr;
    EditorCamera *m_Camera = nullptr;
    EditorEntityFactory *m_EntityFactory = nullptr;
    CommandHistory *m_CommandHistory = nullptr;

    RenderTexture2D m_ViewportTexture = {0};
    uint32_t m_Width = 0, m_Height = 0;
    bool m_Focused = false, m_Hovered = false;

    // Grid
    EditorGrid m_Grid;
    bool m_GridInitialized = false;

    // Sub-systems
    ViewportGizmo m_Gizmo;
    ViewportRenderer m_Renderer;
};
} // namespace CHEngine

#endif // VIEWPANEL_H
