#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include "core/layer/Layer.h"
#include "editor/EditorTypes.h"
#include "editor/camera/EditorCamera.h"
#include "editor/logic/EditorEntityFactory.h"
#include "editor/logic/EditorInput.h"
#include "editor/logic/EditorProjectActions.h"
#include "editor/logic/EditorSceneActions.h"
#include "editor/logic/PanelManager.h"
#include "editor/logic/ProjectManager.h"
#include "editor/logic/SceneSimulationManager.h"
#include "editor/logic/SelectionManager.h"
#include "editor/logic/undo/CommandHistory.h"
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

    // --- Scene Operations ---
    void PlayInRuntime();

    // --- Project Actions ---
    // (Handled by m_ProjectActions)

    // --- Entity/Object Management ---
public:
    // --- Entity/Object Actions ---
    // (Handled by m_EntityFactory)

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
    std::shared_ptr<Scene> GetUIScene();

    enum class SceneContext
    {
        Game,
        UI
    };
    void SetSceneContext(SceneContext context)
    {
        m_CurrentContext = context;
    }
    SceneContext GetSceneContext() const
    {
        return m_CurrentContext;
    }
    std::shared_ptr<Scene> GetCurrentEditingScene()
    {
        return m_CurrentContext == SceneContext::Game ? m_Scene : m_UIScene;
    }

    // (Handled by m_Input)

    // (Handled by m_Interface)

    void InitPanels();

    // --- Member Variables ---
private:
    EditorCamera m_EditorCamera;

    // Managers
    ProjectManager m_ProjectManager;
    SceneSimulationManager m_SimulationManager;
    SelectionManager m_SelectionManager;
    CommandHistory m_CommandHistory;

    // Actions (Godot Style)
    std::unique_ptr<EditorSceneActions> m_SceneActions;
    std::unique_ptr<EditorProjectActions> m_ProjectActions;
    std::unique_ptr<EditorEntityFactory> m_EntityFactory;
    std::unique_ptr<EditorInput> m_Input;
    std::unique_ptr<PanelManager> m_PanelManager;

    bool m_ShowProjectBrowser = true;

    // Scene System
    std::shared_ptr<Scene> m_Scene; // Game Scene
    std::shared_ptr<Scene> m_UIScene;
    SceneContext m_CurrentContext = SceneContext::Game;

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
