#include "editor_layer.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/editor.h"
#include "editor_gui.h"
#include "engine/core/input.h"

#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"

#include "cstdarg"
#include "extras/IconsFontAwesome6.h"
#include "nfd.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/viewport_panel.h"
#include "panels/property_editor.h"
#include "raylib.h"

namespace CHEngine
{
    EditorLayer *EditorLayer::s_Instance = nullptr;

    EditorLayer::EditorLayer() : Layer("EditorLayer")
    {
        s_Instance = this;
        m_Panels = std::make_unique<EditorPanels>();
        m_Panels = std::make_unique<EditorPanels>();
        m_Layout = std::make_unique<EditorLayout>();
        m_Actions = std::make_unique<EditorActions>();

        m_State.DebugRenderFlags.DrawColliders = true;
        m_State.DebugRenderFlags.DrawLights = true;
        m_State.DebugRenderFlags.DrawSpawnZones = true;
    }

    void EditorLayer::OnAttach()
    {
        SetTraceLogCallback(
            [](int logLevel, const char *text, va_list args)
            {
                char buffer[4096];
                vsnprintf(buffer, sizeof(buffer), text, args);
                printf("%s\n", buffer);

                ConsoleLogLevel level = ConsoleLogLevel::Info;
                if (logLevel == LOG_WARNING) level = ConsoleLogLevel::Warn;
                else if (logLevel >= LOG_ERROR) level = ConsoleLogLevel::Error;

                ConsolePanel::AddLog(buffer, level);
            });

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        EditorGUI::ApplyTheme();
        NFD_Init();
        PropertyEditor::Init();

        // Register Panels
        m_Panels->Init();

        m_CommandHistory.SetNotifyCallback(
            []() { CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor..."); });

        // Auto-load last project/scene
        const auto &config = Editor::Get().GetEditorConfig();

        if (config.LoadLastProjectOnStartup && !config.LastProjectPath.empty() &&
            std::filesystem::exists(config.LastProjectPath))
        {
            CH_CORE_INFO("Auto-loading last project: {}", config.LastProjectPath);
            ProjectActions::Open(config.LastProjectPath);

            if (!config.LastScenePath.empty() && std::filesystem::exists(config.LastScenePath))
            {
                CH_CORE_INFO("Auto-loading last scene: {}", config.LastScenePath);
                SceneActions::Open(config.LastScenePath);
            }
        }
        else
        {
            Project::SetActive(nullptr);
        }

        // Ensure layout is initialized
        const char* iniPath = ImGui::GetIO().IniFilename;
        if (iniPath && !std::filesystem::exists(iniPath))
        {
            CH_CORE_INFO("OnAttach: Layout file '{}' not found, will be reset on first frame", iniPath);
            m_State.NeedsLayoutReset = true;
        }

        CH_CORE_INFO("EditorLayer Attached with modular panels.");
    }

    void EditorLayer::OnDetach()
    {
        SetTraceLogCallback(nullptr);
        NFD_Quit();
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        CH_PROFILE_FUNCTION();

        if (Input::IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        if (auto scene = GetActiveScene())
        {
            if (m_SceneState == SceneState::Play)
                scene->OnUpdateRuntime(deltaTime);
            
            m_Panels->OnUpdate(deltaTime);
        }
    }

    void EditorLayer::OnRender()
    {
        ClearBackground(BLACK);
    }

    void EditorLayer::OnImGuiRender()
    {
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
        ImGuizmo::BeginFrame();

        if (m_State.NeedsLayoutReset)
        {
            ResetLayout();
            m_State.NeedsLayoutReset = false;
        }

        if (Project::GetActive())
        {
            if (m_State.FullscreenGame)
            {
                if (auto viewportPanel = m_Panels->Get<ViewportPanel>())
                    viewportPanel->OnImGuiRender(true);
            }
            else
            {
                DrawDockSpace();
            }
        }
        else
        {
            if (auto projectBrowser = m_Panels->Get<ProjectBrowserPanel>())
                projectBrowser->OnImGuiRender();
        }
    }

    void EditorLayer::ResetLayout()
    {
        m_Layout->ResetLayout();
    }

    void EditorLayer::DrawDockSpace()
    {
        m_Layout->BeginWorkspace();
        m_Layout->DrawInterface();

        bool readOnly = m_SceneState == SceneState::Play;
        m_Panels->OnImGuiRender(readOnly);

        DrawScriptUI();

        m_Layout->EndWorkspace();
    }

    void EditorLayer::DrawScriptUI()
    {
        if (m_SceneState != SceneState::Play)
            return;

        auto scene = GetActiveScene();
        if (!scene)
            return;

        scene->GetRegistry().view<NativeScriptComponent>().each(
            [&](auto entity, auto &nsc)
            {
                for (auto &script : nsc.Scripts)
                {
                    if (script.Instance)
                        script.Instance->OnImGuiRender();
                }
            });
    }

    bool EditorLayer::OnProjectOpened(ProjectOpenedEvent &e)
    {
        CH_CORE_INFO("EditorLayer: Handling ProjectOpenedEvent - {}", e.GetPath());

        auto project = Project::GetActive();
        if (project)
        {
            if (auto contentBrowser = m_Panels->Get<ContentBrowserPanel>())
                contentBrowser->SetRootDirectory(Project::GetAssetDirectory());
        }
        return false;
    }

    bool EditorLayer::OnSceneOpened(SceneOpenedEvent &e)
    {
        auto activeScene = GetActiveScene();
        m_Panels->SetContext(activeScene);

        m_State.SelectedEntity = {};

        // Sync project path
        auto project = Project::GetActive();
        if (project && !e.GetPath().empty())
        {
            project->SetActiveScenePath(std::filesystem::relative(e.GetPath(), project->GetProjectDirectory()));
            ProjectActions::Save();
        }
        return false;
    }

    void EditorLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        
        // 1. Scene Management
        dispatcher.Dispatch<SceneOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneOpened));
        dispatcher.Dispatch<ScenePlayEvent>([this](auto& e) { SetSceneState(SceneState::Play); return true; });
        dispatcher.Dispatch<SceneStopEvent>([this](auto& e) { SetSceneState(SceneState::Edit); return true; });

        // 2. Project Management
        dispatcher.Dispatch<ProjectCreatedEvent>([](auto& ev) { ProjectActions::New(ev.GetProjectName(), ev.GetPath()); return true; });
        dispatcher.Dispatch<ProjectOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnProjectOpened));
        dispatcher.Dispatch<AppLaunchRuntimeEvent>([](auto& ev) { ProjectActions::LaunchStandalone(); return true; });

        // 3. Layout/System
        dispatcher.Dispatch<AppResetLayoutEvent>([this](auto& ev) { ResetLayout(); return true; });
        dispatcher.Dispatch<AppSaveLayoutEvent>([this](auto& ev) { m_Layout->SaveDefaultLayout(); return true; });
        dispatcher.Dispatch<SceneChangeRequestEvent>([](auto& ev) { SceneActions::Open(ev.GetPath()); return true; });

        // 4. Selections/Picking
        dispatcher.Dispatch<EntitySelectedEvent>([this](auto& ev) {
            m_State.SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
            m_State.LastHitMeshIndex = ev.GetMeshIndex();
            return false;
        });

        // 5. Hierarchy propagation
        m_Panels->OnEvent(e);
        if (e.Handled) return;
        if (m_Actions->OnEvent(e)) return;

        // 6. Raw Input Overrides
        if (e.GetEventType() == EventType::KeyPressed)
        {
            auto &ke = (KeyPressedEvent &)e;
            if (ke.GetKeyCode() == KEY_ESCAPE && m_State.FullscreenGame)
            {
                m_State.FullscreenGame = false;
                e.Handled = true;
            }
            if (auto activeScene = GetActiveScene())
                activeScene->OnEvent(e);
        }
    }

    CommandHistory &EditorLayer::GetCommandHistory()
    {
        return s_Instance->m_CommandHistory;
    }

    void EditorLayer::SetSceneState(SceneState state)
    {
        if (state == SceneState::Play)
        {
            if (m_SceneState == SceneState::Play) return;

            CH_CORE_INFO("Editor: Play Mode Started");
            m_RuntimeScene = Scene::Copy(m_EditorScene);
            if (m_RuntimeScene)
            {
                m_SceneState = SceneState::Play;
                m_RuntimeScene->OnRuntimeStart();
                m_Panels->SetContext(m_RuntimeScene);
            }
        }
        else
        {
            if (m_SceneState == SceneState::Edit) return;

            CH_CORE_INFO("Editor: Play Mode Stopped");
            if (m_RuntimeScene)
            {
                m_RuntimeScene->OnRuntimeStop();
                m_RuntimeScene = nullptr;
            }

            m_SceneState = SceneState::Edit;
            m_Panels->SetContext(m_EditorScene);
        }
    }

    void EditorLayer::SetScene(std::shared_ptr<Scene> scene)
    {
        m_EditorScene = scene;
        m_State.SelectedEntity = {};
        if (m_SceneState == SceneState::Edit)
            m_Panels->SetContext(m_EditorScene);
    }

    // Simplified Transition logic is now in EditorSceneManager
    // Removed TransitionToPlay/Edit and OnScenePlay/Stop overrides from EditorLayer
} // namespace CHEngine
