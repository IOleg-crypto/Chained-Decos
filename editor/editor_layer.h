#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/core/application.h"
#include "engine/core/base.h"
#include "engine/core/layer.h"
#include "editor/actions/editor_actions.h"
#include "editor_panels.h"
#include "editor_layout.h"
#include "editor_context.h"
#include "engine/graphics/render.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "undo/command_history.h"
#include "imgui.h"

namespace CHEngine
{
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
        virtual void OnEvent(Event &e) override;

        static float GetViewportWidth() { return s_Instance->m_ViewportSize.x; }
        static float GetViewportHeight() { return s_Instance->m_ViewportSize.y; }

        void ResetLayout();
        void SetSceneState(SceneState state);
        SceneState GetSceneState() const { return EditorContext::GetSceneState(); }
        
        static EditorLayer& Get() { return *s_Instance; }
        void SetScene(std::shared_ptr<Scene> scene);
        void DrawDockSpace();
        void DrawScriptUI();

    private:
        void LoadEditorFonts();

    public:
        void SetViewportSize(const ImVec2& size);
        const ImVec2& GetViewportSize() const { return m_ViewportSize; }

    public:
        static EditorLayer *s_Instance;

        static CommandHistory &GetCommandHistory();
        static CommandHistory &History() { return GetCommandHistory(); }
        EditorPanels &GetPanels() { return *m_Panels; }
        
        Entity GetSelectedEntity() const { return EditorContext::GetSelectedEntity(); }
        DebugRenderFlags &GetDebugRenderFlags() { return EditorContext::GetDebugRenderFlags(); }

        void ToggleFullscreenGame(bool enabled) { EditorContext::GetState().FullscreenGame = enabled; }
        bool IsFullscreenGame() const { return EditorContext::GetState().FullscreenGame; }
        bool IsStandaloneActive() const { return EditorContext::GetState().StandaloneActive; }

        std::shared_ptr<Scene> GetActiveScene() const 
        { 
            return (EditorContext::GetSceneState() == SceneState::Play) ? m_RuntimeScene : m_EditorScene; 
        }

    private:
        bool OnProjectOpened(ProjectOpenedEvent &e);
        bool OnSceneOpened(SceneOpenedEvent &e);

    private:
        std::unique_ptr<EditorPanels> m_Panels;
        std::unique_ptr<EditorLayout> m_Layout;
        std::unique_ptr<EditorActions> m_Actions;

        std::shared_ptr<Scene> m_EditorScene;
        std::shared_ptr<Scene> m_RuntimeScene;

        CommandHistory m_CommandHistory;
        ImVec2 m_ViewportSize = {1280, 720};
    };
} // namespace CHEngine

#endif // CH_EDITOR_LAYER_H
