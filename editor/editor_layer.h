#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
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

    template <typename T, typename... Args> Ref<T> AddPanel(Args &&...args)
    {
        auto panel = std::make_shared<T>(std::forward<Args>(args)...);
        m_Panels.push_back(panel);
        return panel;
    }

    template <typename T> Ref<T> GetPanel()
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

private:
    std::vector<Ref<Panel>> m_Panels;

    Entity m_SelectedEntity;
    SceneState m_SceneState = SceneState::Edit;
    DebugRenderFlags m_DebugRenderFlags;
    int m_LastHitMeshIndex = -1;
    Ref<Scene> m_EditorScene;

private:
    CommandHistory m_CommandHistory;
};
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
