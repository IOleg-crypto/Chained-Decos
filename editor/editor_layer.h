#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
// Removed redundant include: engine/graphics/render.h
#include "editor/actions/editor_actions.h"
#include "editor/editor_panels.h"
#include "editor/ui/editor_layout.h"
#include "editor_types.h"
#include "engine/graphics/render_types.h"
#include "engine/scene/scene.h"
#include "filesystem"
#include "imgui.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/inspector_panel.h"
#include "panels/profiler_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/project_settings_panel.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/viewport_panel.h"
#include "raylib.h"
#include "undo/command_history.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"

namespace CHEngine
{
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

        void ResetLayout();
        void DrawDockSpace();
        void DrawScriptUI();

    private:
        bool OnProjectOpened(ProjectOpenedEvent &e);
        bool OnSceneOpened(SceneOpenedEvent &e);
        bool OnScenePlay(ScenePlayEvent &e);
        bool OnSceneStop(SceneStopEvent &e);

    public:
        static EditorLayer *s_Instance;
        static ImVec2 s_ViewportSize;

        static CommandHistory &GetCommandHistory();
        static SceneState GetSceneState();
        static EditorLayer &Get();

        EditorPanels &GetPanels()
        {
            return *m_Panels;
        }
        Entity GetSelectedEntity() const
        {
            return m_SelectedEntity;
        }

        Camera3D GetActiveCamera();
        DebugRenderFlags &GetDebugRenderFlags()
        {
            return m_DebugRenderFlags;
        }

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
        std::unique_ptr<EditorPanels> m_Panels;
        std::unique_ptr<EditorLayout> m_Layout;
        std::unique_ptr<EditorActions> m_Actions;

        Entity m_SelectedEntity;
        SceneState m_SceneState = SceneState::Edit;
        bool m_FullscreenGame = false;
        bool m_StandaloneActive = false;
        bool m_NeedsLayoutReset = false;
        int m_LastHitMeshIndex = -1;
        std::shared_ptr<Scene> m_EditorScene;

        // Raylib Debug Render Flags
        DebugRenderFlags m_DebugRenderFlags;

    private:
        CommandHistory m_CommandHistory;
    };
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
