#ifndef CD_EDITOR_EDITOR_LAYER_H
#define CD_EDITOR_EDITOR_LAYER_H

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
#include "engine/core/layer/layer.h"
#include "engine/scene/core/entity.h"
#include "engine/scene/core/scene.h"
#include "events/event.h"
#include "events/key_event.h"
#include "events/mouse_event.h"
#include "project/project.h"


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

    void SetActiveTool(Tool tool)
    {
        m_ActiveTool = tool;
    }

private:
    void InitPanels();
    void InitServices();

    // Editor Logic (Ownership)
    std::unique_ptr<EditorSceneActions> m_SceneActions;
    std::unique_ptr<EditorProjectActions> m_ProjectActions;
    std::unique_ptr<EditorEntityFactory> m_EntityFactory;
    std::unique_ptr<EditorInput> m_Input;
    std::unique_ptr<PanelManager> m_PanelManager;

    // Simulation & Camera
    SceneSimulationManager m_SimulationManager;
    EditorCamera m_EditorCamera;
    CHD::RuntimeLayer *m_RuntimeLayer = nullptr;

    // State
    Tool m_ActiveTool = Tool::Select;
    bool m_ShowProjectBrowser = true;
};
} // namespace CHEngine

#endif // CD_EDITOR_EDITOR_LAYER_H
