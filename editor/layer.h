#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include <memory>
#include <string>
#include "imgui.h"

#include "engine/scene/scene.h"
#include "engine/scene/scene_context.h"
#include "editor/project_manager.h"
#include "editor/types.h"
#include "editor/scene_manager.h"
#include "engine/app/application.h"
#include "engine/core/layer.h"
#include "editor/layout.h"
#include "editor/panels.h"
#include "editor/undo/command_history.h"

namespace Chained
{

class ProjectSelectorUI;

class EditorLayer : public Layer
{
public:
    static EditorLayer& Get() { return *s_Instance; }

    EditorLayer();
    virtual ~EditorLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnRender(Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event& e) override;

    float GetViewportWidth() const { return m_ViewportSize.x; }
    float GetViewportHeight() const { return m_ViewportSize.y; }

    void ResetLayout();
    
    EditorSceneManager& GetSceneManager() { return *m_SceneManager; }
    EditorProjectManager& GetProjectManager() { return *m_ProjectManager; }
    

    Entity GetSelectedEntity() const { return m_EditorState.SelectedEntity; }
    void SetSelectedEntity(Entity entity) { m_EditorState.SelectedEntity = entity; }

    DebugRenderFlags& GetDebugRenderFlags() { return m_EditorState.DebugRenderFlags; }
    EditorState& GetEditorState() { return m_EditorState; }
    
    SceneState GetSceneState() const { return m_SceneManager->GetSceneState(); }
    void SetSceneState(SceneState state) { m_SceneManager->SetSceneState(state); }

private:
    void LoadEditorFonts();
    void DrawLoadingOverlay(const char* title, const char* status);

public:
    CommandHistory& GetCommandHistory();
    EditorPanels& GetPanels() { return *m_Panels; }

    static void ReparentEntity(Entity child, Entity parent);

    ImVec2 GetViewportSize() const { return m_ViewportSize; }
    ImVec2& GetViewportSizeRef() { return m_ViewportSize; }
    void OnViewportResized(const ImVec2& size) { m_ViewportSize = size; }
    void SetLastScenePath(const std::string& path) { m_Config.LastScenePath = path; }

    void LoadConfig();
    void SaveConfig();
    const EditorConfig& GetConfig() const { return m_Config; }
    EditorConfig& GetConfig() { return m_Config; }

    std::shared_ptr<Scene> GetActiveScene() const;

private:
    // Resolved once in the constructor (ServiceLocator is already locked by then —
    // see Application::Application) and reused for the layer's whole lifetime.
    SceneContext m_Context;

    EditorConfig m_Config;
    EditorState m_EditorState;

    std::unique_ptr<EditorLayout> m_Layout;
    std::unique_ptr<EditorPanels> m_Panels;
    std::unique_ptr<EditorProjectManager> m_ProjectManager;
    std::unique_ptr<EditorSceneManager> m_SceneManager;
    std::unique_ptr<ProjectSelectorUI> m_ProjectSelectorUI;
    CommandHistory m_CommandHistory;
    
    std::string m_PendingSceneTransitionPath;
    ImVec2 m_ViewportSize = {1280, 720};
    
    static inline EditorLayer* s_Instance = nullptr;
};
} // namespace Chained

#endif // CH_EDITOR_LAYER_H