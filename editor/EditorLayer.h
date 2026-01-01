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
#include "editor/panels/ProjectBrowserPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include "project/Project.h"
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

    // --- Layer Lifecycle ---
public:
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event &event) override;

    // --- Scene Lifecycle ---
public:
    void OnScenePlay();
    void OnSceneStop();

    // --- Scene Commands ---
public:
    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    void PlayInRuntime();

    // --- Project Management ---
public:
    void NewProject(const std::string &name, const std::string &location);
    void OpenProject(const std::string &projectPath);
    void CloseProject();

    // --- Entity/Object Management ---
public:
    void AddObject(const MapObjectData &data);
    void CreateEntity();
    void DeleteEntity(entt::entity entity);
    void DeleteObject(int index);
    void AddModel();
    void AddUIElement(const std::string &type);
    void OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition);

    // --- Environment Management ---
public:
    void LoadSkybox(const std::string &path = "");
    void ApplySkybox(const std::string &path);

    // --- Getters & Setters ---
public:
    SceneState GetSceneState() const;
    SelectionManager &GetSelectionManager();
    MapObjectData *GetSelectedObject();
    UIElementData *GetSelectedUIElement();
    Tool GetActiveTool() const;
    void SetActiveTool(Tool tool);
    std::shared_ptr<Scene> GetActiveScene();

    // --- Input Handling ---
private:
    bool OnKeyPressed(KeyPressedEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

    // --- UI Helpers ---
private:
    void UI_DrawDockspace();

    // --- Member Variables ---
private:
    EditorCamera m_EditorCamera;

    // Panels
    std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<ViewportPanel> m_ViewportPanel;
    std::unique_ptr<AssetBrowserPanel> m_AssetBrowserPanel;
    std::unique_ptr<ConsolePanel> m_ConsolePanel;
    std::unique_ptr<ToolbarPanel> m_ToolbarPanel;
    std::unique_ptr<ProjectBrowserPanel> m_ProjectBrowserPanel;
    std::unique_ptr<MenuBarPanel> m_MenuBarPanel;

    // Managers
    ProjectManager m_ProjectManager;
    SceneSimulationManager m_SimulationManager;
    SelectionManager m_SelectionManager;
    CommandHistory m_CommandHistory;

    bool m_ShowProjectBrowser = true;

    // Scene System
    std::shared_ptr<Scene> m_Scene;

    // Viewport State
    ImVec2 m_ViewportSize = {0.0f, 0.0f};
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;
    bool m_CursorLocked = true;
    bool m_ShowProjectSettings = false;

    CHD::RuntimeLayer *m_RuntimeLayer = nullptr;
    Tool m_ActiveTool = Tool::MOVE;
};
} // namespace CHEngine

#endif // EDITOR_LAYER_H
