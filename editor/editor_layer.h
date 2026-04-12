#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "editor_context.h"
#include "editor_layout.h"
#include "editor_panels.h"
#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "undo/command_history.h"
#include <filesystem>
#include <memory>
#include <future>

namespace CHEngine
{

// Owns the editor scene pair, viewport state, and project/scene transition flow.
class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnRender(Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event& e) override;

    // Returns the viewport width currently tracked by the editor.
    static float GetViewportWidth()
    {
        return s_Instance->m_ViewportSize.x;
    }
    // Returns the viewport height currently tracked by the editor.
    static float GetViewportHeight()
    {
        return s_Instance->m_ViewportSize.y;
    }

    // Resets the editor layout to the default dock structure.
    void ResetLayout();
    // Requests a transition between Edit and Play scene state.
    void SetSceneState(SceneState state);
    SceneState GetSceneState() const
    {
        return EditorContext::GetSceneState();
    }

    static EditorLayer& Get()
    {
        return *s_Instance;
    }
    // Replaces the current editor scene and updates the runtime scene as needed.
    void SetScene(std::shared_ptr<Scene> scene);
    // Draws the main editor docking root.
    void DrawDockSpace();

private:
    void LoadEditorFonts();
    void StartPlayModeTransition();
    void UpdatePlayModeTransition();
    void CancelPlayModeTransition(bool waitForCopy);
    void StartSceneOpenTransition(const std::filesystem::path& path);
    void UpdateSceneOpenTransition();
    void CancelSceneOpenTransition(bool waitForCopy);
    void DrawPlayModeLoadingOverlay();
    void DrawSceneOpenLoadingOverlay();

public:
    // Updates the viewport size used by editor rendering and picking.
    void SetViewportSize(const ImVec2& size);
    const ImVec2& GetViewportSize() const
    {
        return m_ViewportSize;
    }

    // File and project operations.
    void NewProject();
    void NewProject(const std::string& name, const std::string& path);
    void OpenProject();
    void OpenProject(const std::filesystem::path& path);
    void SaveProject();
    void LaunchStandalone();

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path& path);
    void SaveScene();
    void SaveSceneAs();
    void AutoSaveScene();

public:
    static EditorLayer* s_Instance;

    static CommandHistory& GetCommandHistory();
    static CommandHistory& History()
    {
        return GetCommandHistory();
    }
    EditorPanels& GetPanels()
    {
        return *m_Panels;
    }

    Entity GetSelectedEntity() const
    {
        return EditorContext::GetSelectedEntity();
    }

    static void ReparentEntity(Entity child, Entity parent);

    DebugRenderFlags& GetDebugRenderFlags()
    {
        auto activeScene = GetActiveScene();
        if (activeScene)
        {
            return activeScene->GetSettings().DebugFlags;
        }
        return EditorContext::GetDebugRenderFlags();
    }

    void ToggleFullscreenGame(bool enabled)
    {
        EditorContext::GetState().FullscreenGame = enabled;
    }
    bool IsFullscreenGame() const
    {
        return EditorContext::GetState().FullscreenGame;
    }
    bool IsStandaloneActive() const
    {
        return EditorContext::GetState().StandaloneActive;
    }

    struct EditorLayerConfig
    {
        // Persistent editor preferences and last-opened paths.
        std::string LastProjectPath = "";
        std::string LastScenePath = "";
        bool LoadLastProjectOnStartup = false;
        bool AutoSaveEnabled = true;
        float AutoSaveInterval = 300.0f; // 5 minutes (300 seconds)
    };

    // Loads the editor config from disk.
    void LoadConfig();
    // Saves the editor config to disk.
    void SaveConfig();
    const EditorLayerConfig& GetConfig() const
    {
        return m_Config;
    }
    EditorLayerConfig& GetConfig()
    {
        return m_Config;
    }
    void SetLastProjectPath(const std::string& path)
    {
        m_Config.LastProjectPath = path;
    }
    void SetLastScenePath(const std::string& path)
    {
        m_Config.LastScenePath = path;
    }

    // Returns the scene currently being edited or played, or null while transitions are in flight.
    std::shared_ptr<Scene> GetActiveScene() const
    {
        if (m_IsPlayModeLoading || m_IsSceneOpenLoading)
        {
            return nullptr;
        }

        return (EditorContext::GetSceneState() == SceneState::Play) ? m_RuntimeScene : m_EditorScene;
    }

private:
    EditorLayerConfig m_Config;
    bool OnProjectOpened(ProjectOpenedEvent& e);
    bool OnSceneOpened(SceneOpenedEvent& e);

private:
    std::unique_ptr<EditorLayout> m_Layout;
    std::unique_ptr<EditorPanels> m_Panels;

    std::shared_ptr<Scene> m_EditorScene;
    std::shared_ptr<Scene> m_RuntimeScene;
    std::future<std::shared_ptr<Scene>> m_PlayModeCopyFuture;
    std::future<std::shared_ptr<Scene>> m_SceneOpenFuture;
    std::filesystem::path m_PendingSceneOpenPath;

    CommandHistory m_CommandHistory;
    ImVec2 m_ViewportSize = {1280, 720};
    float m_AutoSaveTimer = 0.0f;
    float m_LastAutoSaveTime = 0.0f;
    bool m_PlayModeStartRequested = false;
    bool m_IsPlayModeLoading = false;
    bool m_PlayModeSceneReady = false;
    bool m_IsSceneOpenLoading = false;
    bool m_SceneOpenSceneReady = false;

    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
};
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
