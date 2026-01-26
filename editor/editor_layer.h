#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "engine/render/render.h"
#include "engine/scene/scene.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/inspector_panel.h"
#include "panels/profiler_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/project_settings_panel.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/viewport_panel.h"
#include "undo/command_history.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"
#include <filesystem>
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{
enum class SceneState : uint8_t
{
    Edit = 0,
    Play = 1
};

class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event &e) override;

    static float GetViewportHeight()
    {
        return s_ViewportSize.y;
    }

    static std::shared_ptr<Scene> GetActiveScene()
    {
        return Application::Get().GetActiveScene();
    }

    template <typename T, typename... Args> std::shared_ptr<T> AddPanel(Args &&...args)
    {
        auto panel = std::make_shared<T>(std::forward<Args>(args)...);
        m_Panels.push_back(panel);
        return panel;
    }

    template <typename T> std::shared_ptr<T> GetPanel()
    {
        for (auto &panel : m_Panels)
        {
            if (auto p = std::dynamic_pointer_cast<T>(panel))
                return p;
        }
        return nullptr;
    }

private:
    bool OnProjectOpened(ProjectOpenedEvent &e);
    bool OnSceneOpened(SceneOpenedEvent &e);
    bool OnScenePlay(ScenePlayEvent &e);
    bool OnSceneStop(SceneStopEvent &e);

    void ResetLayout();
    void LaunchStandalone();

    void SetDarkThemeColors();
    Camera3D GetActiveCamera();

    void UI_DrawDockSpace();
    void UI_DrawPanels();
    void UI_DrawScriptUI();

public:
    static CommandHistory &GetCommandHistory();
    static SceneState GetSceneState();
    static EditorLayer &Get();

    void ToggleFullscreenGame(bool enabled)
    {
        m_FullscreenGame = enabled;
    }
    bool IsFullscreenGame() const
    {
        return m_FullscreenGame;
    }
    bool IsStandaloneActive() const
    {
        return m_StandaloneActive;
    }

private:
    std::vector<std::shared_ptr<Panel>> m_Panels;

    Entity m_SelectedEntity;
    SceneState m_SceneState = SceneState::Edit;
    bool m_FullscreenGame = false;
    bool m_StandaloneActive = false;
    int m_LastHitMeshIndex = -1;
    std::shared_ptr<Scene> m_EditorScene;

    // Raylib Debug Render Flags
    DebugRenderFlags m_DebugRenderFlags;

private:
    CommandHistory m_CommandHistory;

    static EditorLayer *s_Instance;
    static ImVec2 s_ViewportSize;
};
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
