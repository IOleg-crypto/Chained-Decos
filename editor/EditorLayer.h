#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include "core/layer/Layer.h"
#include "editor/EditorTypes.h"
#include "editor/camera/EditorCamera.h"
#include "editor/logic/ProjectManager.h"
#include "editor/logic/SceneSimulationManager.h"
#include "editor/logic/SelectionManager.h"
#include "editor/logic/undo/CommandHistory.h"
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/MenuBarPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include "scene/core/Entity.h"
#include "scene/core/Scene.h"
#include "scene/resources/map/SceneLoader.h"


#include <imgui.h>
#include <memory>

namespace CHD
{
class RuntimeLayer;
}

namespace CHEngine
{
class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender();
    virtual void OnEvent(Event &event) override;

    void OnScenePlay();
    void OnSceneStop();

    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    void AddModel();
    void AddUIElement(const std::string &type);

    // Object Management Helpers (Undoable)
    void AddObject(const MapObjectData &data);
    void DeleteObject(int index);
    void OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition);

    void LoadSkybox(const std::string &path = "");
    void ApplySkybox(const std::string &path);

    SceneState GetSceneState() const
    {
        return m_SimulationManager.GetSceneState();
    }

    SelectionManager &GetSelectionManager()
    {
        return m_SelectionManager;
    }

    MapObjectData *GetSelectedObject();
    UIElementData *GetSelectedUIElement();

    Tool GetActiveTool() const
    {
        return m_ActiveTool;
    }
    void SetActiveTool(Tool tool)
    {
        m_ActiveTool = tool;
    }

    bool OnKeyPressed(KeyPressedEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

    void UI_DrawDockspace();

    // Scene System Accessors
    std::shared_ptr<Scene> GetActiveScene()
    {
        return m_Scene;
    }

private:
    EditorCamera m_EditorCamera;

    // Panels
    std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<ViewportPanel> m_ViewportPanel;
    std::unique_ptr<AssetBrowserPanel> m_AssetBrowserPanel;
    std::unique_ptr<ConsolePanel> m_ConsolePanel;
    std::unique_ptr<ToolbarPanel> m_ToolbarPanel;
    std::unique_ptr<MenuBarPanel> m_MenuBarPanel;

    // Managers
    ProjectManager m_ProjectManager;
    SceneSimulationManager m_SimulationManager;
    SelectionManager m_SelectionManager;
    CommandHistory m_CommandHistory;

    // Scene System (new architecture)
    std::shared_ptr<Scene> m_Scene;

    // Legacy scene system (will be migrated)
    std::shared_ptr<GameScene> m_ActiveScene;
    std::shared_ptr<GameScene> m_EditorScene;

    // Viewport
    ImVec2 m_ViewportSize = {0.0f, 0.0f};
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

    CHD::RuntimeLayer *m_RuntimeLayer = nullptr;
    Tool m_ActiveTool = Tool::MOVE;
    bool m_ShowProjectSettings = false;
};
} // namespace CHEngine

#endif // EDITOR_LAYER_H
