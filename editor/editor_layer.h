#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include <filesystem>
#include <memory>
#include <future>
#include <vector>
#include <string>


#include "editor_context.h"
#include "launcher/editor_launcher.h"
#include "editor_project_manager.h"
#include "editor_scene_manager.h"
#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "editor_layout.h"
#include "editor_panels.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"
#include "undo/command_history.h"

namespace CHEngine
{

struct EditorLayerConfig
{
    std::string LastProjectPath = "";
    std::string LastScenePath = "";
    bool LoadLastProjectOnStartup = true;
    bool AutoSaveEnabled = true;
    float AutoSaveInterval = 300.0f;
    std::vector<std::string> RecentProjects; // Ordered list of recently opened project paths
};

// Owns the editor scene pair, viewport state, and project/scene transition flow.
class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer();

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
    
    SceneState GetSceneState() const
    {
        return EditorContext::GetSceneState();
    }

    static EditorLayer& Get()
    {
        return *s_Instance;
    }
    // Draws the main editor docking root.
    void DrawDockSpace();

    // File and project operations (delegated to ProjectManager).
    EditorProjectManager& GetProjectManager() { return *m_ProjectManager; }

    // Scene operations (delegated to SceneManager).
    EditorSceneManager& GetSceneManager() { return *m_SceneManager; }

    void LaunchStandalone();

private:
    void LoadEditorFonts();
    void DrawLoadingOverlay(const char* title, const char* status);

public:
    static EditorLayer* s_Instance;
public:
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

    const ImVec2& GetViewportSize() const { return m_ViewportSize; }
    void SetViewportSize(const ImVec2& size) { m_ViewportSize = size; }
    void SetLastScenePath(const std::string& path)
    {
        m_Config.LastScenePath = path;
    }

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

    // Returns the scene currently being edited or played, or null while transitions are in flight.
    std::shared_ptr<Scene> GetActiveScene() const;

private:
    EditorLayerConfig m_Config;

private:
    std::unique_ptr<EditorLayout> m_Layout;
    std::unique_ptr<EditorPanels> m_Panels;
    std::unique_ptr<EditorProjectManager> m_ProjectManager;
    std::unique_ptr<EditorSceneManager> m_SceneManager;

    CommandHistory m_CommandHistory;
    ImVec2 m_ViewportSize = {1280, 720};
};
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
