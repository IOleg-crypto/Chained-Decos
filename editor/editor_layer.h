#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "engine/scene/scene.h"
#include "logic/undo/command_history.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/inspector_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/viewport_panel.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"
#include <filesystem>
#include <raylib.h>

namespace CH
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

private:
    void NewProject();
    void OpenProject();
    void OpenProject(const std::filesystem::path &path);
    void SaveProject();

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path &path);
    void SaveScene();
    void SaveSceneAs();

    void ResetLayout();

    void OnScenePlay();
    void OnSceneStop();

    void SetDarkThemeColors();
    Camera3D GetActiveCamera();

    void UI_DrawMenuBar();
    void UI_DrawDockSpace();
    void UI_DrawPanels();

public:
    static CommandHistory &GetCommandHistory();

private:
    EditorCamera m_EditorCamera;
    Ref<Scene> m_ActiveScene;
    SceneState m_SceneState = SceneState::Edit;

private:
    EditorGizmo m_Gizmo;
    GizmoType m_CurrentTool = GizmoType::SELECT;
    CommandHistory m_CommandHistory;

private:
    ProjectBrowserPanel m_ProjectBrowserPanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    ConsolePanel m_ConsolePanel;
    ViewportPanel m_ViewportPanel;
    EnvironmentPanel m_EnvironmentPanel;
    bool m_ShowContentBrowser = true;
};
} // namespace CH

#endif // CH_EDITOR_LAYER_H
