#ifndef CD_EDITOR_EDITOR_LAYER_H
#define CD_EDITOR_EDITOR_LAYER_H

#include "core/layer/layer.h"
#include "editor/camera/editor_camera.h"
#include "editor/editor_types.h"
#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/editor_input.h"
#include "editor/logic/editor_project_actions.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/panel_manager.h"
#include "editor/logic/project_manager.h"
#include "editor/logic/scene_simulation_manager.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"
#include "editor/panels/viewport_panel.h"
#include "events/event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include "project/project.h"
#include "scene/core/entity.h"
#include "scene/core/scene.h"

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

public:
    void LoadSkybox(const std::string &path = "");
    void ApplySkybox(const std::string &path);

    // --- Getters & Setters ---
public:
    SceneState GetSceneState() const;
    SelectionManager &GetSelectionManager();
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

    void InitPanels();

private:
    EditorCamera m_EditorCamera;

    // Managers
    ProjectManager m_ProjectManager;
    SceneSimulationManager m_SimulationManager;
    SelectionManager m_SelectionManager;
    CommandHistory m_CommandHistory;

    // Actions (Godot Style)
    std::unique_ptr<EditorSceneManager> m_SceneManager;
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

#endif // CD_EDITOR_EDITOR_LAYER_H
