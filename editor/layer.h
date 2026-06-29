#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include <memory>
#include <vector>
#include <string>
#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene.h"
#include "project_manager.h"
#include "scene_manager.h"
#include "engine/app/application.h"
#include "engine/core/layer.h"
#include "layout.h"
#include "panels.h"
#include "imgui.h"
#include "undo/command_history.h"

namespace Chained {

enum class SceneState : uint8_t { Edit = 0, Play = 1 , Simulate = 2};

struct EditorState {
    Entity SelectedEntity;
    bool FullscreenGame = false;
    bool StandaloneActive = false;
    bool NeedsLayoutReset = false;
    int LastHitMeshIndex = -1;
    DebugRenderFlags DebugRenderFlags;
    bool IsLoading = false;
    std::string LoadingStatus ;
};
} // namespace Chained


namespace Chained
{


class ProjectSelectorUI;

// Owns the editor scene pair, viewport state, and project/scene transition flow.
class EditorLayer : public Layer
{
public:
    static EditorLayer& Get() { return *s_Instance; }

    EditorLayer(Application& app);
    virtual ~EditorLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnRender(Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event& e) override;

    // Returns the view port width currently tracked by the editor.
    float GetViewportWidth() const
    {
        return m_ViewportSize.x;
    }
    // Returns the view port height currently tracked by the editor.
    float GetViewportHeight() const
    {
        return m_ViewportSize.y;
    }

    // Resets the editor layout to the default dock structure.
    void ResetLayout();
    
    // Project/Scene operations.
    EditorSceneManager& GetSceneManager() { return *m_SceneManager; }
    EditorProjectManager& GetProjectManager() { return *m_ProjectManager; }

    void LaunchStandalone();

    SceneState GetSceneState() const { return m_SceneState; }
    void SetSceneState(SceneState state) { m_SceneState = state; }

    Entity GetSelectedEntity() const { return m_EditorState.SelectedEntity; }
    void SetSelectedEntity(Entity entity) { m_EditorState.SelectedEntity = entity; }

    DebugRenderFlags& GetDebugRenderFlags() { return m_EditorState.DebugRenderFlags; }
    EditorState& GetEditorState() { return m_EditorState; }

private:
    void LoadEditorFonts();
    void DrawLoadingOverlay(const char* title, const char* status);
public:
    CommandHistory& GetCommandHistory();
    EditorPanels& GetPanels()
    {
        return *m_Panels;
    }

    static void ReparentEntity(Entity child, Entity parent);

    ImVec2 GetViewportSize() const { return m_ViewportSize; }
    void OnViewportResized(const ImVec2& size) { m_ViewportSize = size; }
    void SetLastScenePath(const std::string& path)
    {
        m_Config.LastScenePath = path;
    }

    // Loads the editor config from disk.
    void LoadConfig();
    // Saves the editor config to disk.
    void SaveConfig();
    const EditorConfig& GetConfig() const
    {
        return m_Config;
    }
    EditorConfig& GetConfig()
    {
        return m_Config;
    }

    // Returns the scene currently being edited or played, or null while transitions are in flight.
    std::shared_ptr<Scene> GetActiveScene() const;

private:
    EditorConfig m_Config;

private:
    EditorState m_EditorState;
    SceneState m_SceneState = SceneState::Edit;
    std::unique_ptr<EditorLayout> m_Layout;
    std::unique_ptr<EditorPanels> m_Panels;
    std::unique_ptr<EditorProjectManager> m_ProjectManager;
    std::unique_ptr<EditorSceneManager> m_SceneManager;
    std::unique_ptr<ProjectSelectorUI> m_ProjectSelectorUI;
    CommandHistory m_CommandHistory;
    std::string m_PendingSceneTransitionPath;
    ImVec2 m_ViewportSize = {1280, 720};
    
    static EditorLayer* s_Instance;
};
} // namespace Chained

#endif // CH_EDITOR_LAYER_H
