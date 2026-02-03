#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "editor/actions/editor_actions.h"
#include "editor_panels.h"
#include "editor_layout.h"
#include "engine/graphics/render_types.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "undo/command_history.h"
#include "imgui.h"

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

        static float GetViewportWidth() { return s_Instance->m_ViewportSize.x; }
        static float GetViewportHeight() { return s_Instance->m_ViewportSize.y; }

        void ResetLayout();
        void SetSceneState(SceneState state);
        SceneState GetSceneState() const { return m_SceneState; }
        
        static EditorLayer& Get() { return *s_Instance; }
        void SetScene(std::shared_ptr<Scene> scene);
        void DrawDockSpace();
        void DrawScriptUI();

        void SetViewportSize(const ImVec2& size) { m_ViewportSize = size; }
        const ImVec2& GetViewportSize() const { return m_ViewportSize; }

    public:
        static EditorLayer *s_Instance;

        static CommandHistory &GetCommandHistory();
        static CommandHistory &History() { return GetCommandHistory(); }
        EditorPanels &GetPanels() { return *m_Panels; }
        
        SceneState GetState() const { return m_SceneState; }
        
        Entity GetSelectedEntity() const { return m_State.SelectedEntity; }
        DebugRenderFlags &GetDebugRenderFlags() { return m_State.DebugRenderFlags; }

        void ToggleFullscreenGame(bool enabled) { m_State.FullscreenGame = enabled; }
        bool IsFullscreenGame() const { return m_State.FullscreenGame; }
        bool IsStandaloneActive() const { return m_State.StandaloneActive; }

        std::shared_ptr<Scene> GetActiveScene() const 
        { 
            return (m_SceneState == SceneState::Play) ? m_RuntimeScene : m_EditorScene; 
        }

    private:
        bool OnProjectOpened(ProjectOpenedEvent &e);
        bool OnSceneOpened(SceneOpenedEvent &e);

    private:
        std::unique_ptr<EditorPanels> m_Panels;
        std::unique_ptr<EditorLayout> m_Layout;
        std::unique_ptr<EditorActions> m_Actions;

        SceneState m_SceneState = SceneState::Edit;
        std::shared_ptr<Scene> m_EditorScene;
        std::shared_ptr<Scene> m_RuntimeScene;

        struct EditorState
        {
            Entity SelectedEntity;
            bool FullscreenGame = false;
            bool StandaloneActive = false;
            bool NeedsLayoutReset = false;
            int LastHitMeshIndex = -1;
            DebugRenderFlags DebugRenderFlags;
        } m_State;

        CommandHistory m_CommandHistory;
        ImVec2 m_ViewportSize = {1280, 720};
    };
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
