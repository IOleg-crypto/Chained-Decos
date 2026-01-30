#include "editor_layer.h"
#include "actions/editor_actions.h"
#include "editor/actions/editor_actions.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/editor.h"
#include "editor/editor_panels.h"
#include "editor/ui/editor_layout.h"
#include "editor_panels.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "ui/editor_gui.h"

#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"

#include "cstdarg"
#include "extras/iconsfontawesome6.h"
#include "imgui.h"
#include "nfd.h"
#include "panels/console_panel.h"
#include "panels/property_editor.h"
#include "raylib.h"


namespace CHEngine
{
    EditorLayer *EditorLayer::s_Instance = nullptr;
    ImVec2 EditorLayer::s_ViewportSize = {1280, 720};

    EditorLayer::EditorLayer() : Layer("EditorLayer")
    {
        s_Instance = this;
        m_Panels = std::make_unique<EditorPanels>();
        m_Layout = std::make_unique<EditorLayout>();
        m_Actions = std::make_unique<EditorActions>();

        m_DebugRenderFlags.DrawColliders = true;
        m_DebugRenderFlags.DrawLights = true;
        m_DebugRenderFlags.DrawSpawnZones = true;
    }

    void EditorLayer::OnAttach()
    {
        SetTraceLogCallback(
            [](int logLevel, const char *text, va_list args)
            {
                char buffer[4096];
                vsnprintf(buffer, sizeof(buffer), text, args);

                // Also print to standard console
                printf("%s\n", buffer);

                ConsoleLogLevel level = ConsoleLogLevel::Info;
                if (logLevel == LOG_WARNING)
                    level = ConsoleLogLevel::Warn;
                else if (logLevel >= LOG_ERROR)
                    level = ConsoleLogLevel::Error;

                ConsolePanel::AddLog(buffer, level);
            });

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Set font
        float fontSize = 16.0f;
        io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/engine/resources/font/lato/lato-bold.ttf", fontSize);

        // Add FontAwesome icons
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.GlyphMinAdvanceX = fontSize;
        io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/engine/resources/font/fa-solid-900.ttf", fontSize,
                                     &icons_config, icons_ranges);

        EditorUI::GUI::SetDarkThemeColors();

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
            CH_CORE_INFO("OnAttach: No project to auto-load");
            Project::SetActive(nullptr);
        }

        // Ensure layout is initialized
        if (!std::filesystem::exists("imgui.ini"))
        {
            CH_CORE_INFO("OnAttach: Layout will be reset on first frame");
            m_NeedsLayoutReset = true;
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

        auto activeScene = Application::Get().GetActiveScene();
        if (!activeScene)
            return;

        if (m_SceneState == SceneState::Edit)
        {
            activeScene->OnUpdateEditor(deltaTime);
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

        if (m_NeedsLayoutReset)
        {
            m_Layout->ResetLayout();
            m_NeedsLayoutReset = false;
        }

        if (Project::GetActive())
        {
            if (m_FullscreenGame)
            {
                auto viewportPanel = m_Panels->Get<ViewportPanel>();
                if (viewportPanel)
                    viewportPanel->OnImGuiRender(true);
            }
            else
            {
                m_Layout->BeginWorkspace();

                m_Layout->DrawInterface();

                bool readOnly = m_SceneState == SceneState::Play;
                m_Panels->OnImGuiRender(readOnly);

                DrawScriptUI();

                m_Layout->EndWorkspace();
            }
        }
        else
        {
            if (auto projectBrowser = m_Panels->Get<ProjectBrowserPanel>())
                projectBrowser->OnImGuiRender();
        }
    }

    void EditorLayer::DrawScriptUI()
    {
        if (m_SceneState != SceneState::Play)
            return;

        auto scene = Application::Get().GetActiveScene();
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

        // Project is already loaded by EditorUtils::ProjectActions::OpenProject
        // or by the ProjectBrowserPanel. Just update UI panels.

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
        auto activeScene = Application::Get().GetActiveScene();
        m_Panels->SetContext(activeScene);

        m_SelectedEntity = {};

        // Sync project path
        auto project = Project::GetActive();
        if (project && !e.GetPath().empty())
        {
            project->SetActiveScenePath(std::filesystem::relative(e.GetPath(), project->GetProjectDirectory()));
            ProjectActions::Save();
        }
        return false;
    }

    void EditorLayer::OnEvent(CHEngine::Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<ProjectOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnProjectOpened));
        dispatcher.Dispatch<ProjectCreatedEvent>(
            [this](ProjectCreatedEvent &ev)
            {
                CH_CORE_INFO("EditorLayer: Handling ProjectCreatedEvent");
                ProjectActions::New(ev.GetProjectName(), ev.GetPath());
                return true;
            });
        dispatcher.Dispatch<SceneOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneOpened));
        dispatcher.Dispatch<ScenePlayEvent>(CH_BIND_EVENT_FN(EditorLayer::OnScenePlay));
        dispatcher.Dispatch<SceneStopEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneStop));
        dispatcher.Dispatch<AppLaunchRuntimeEvent>(
            [this](AppLaunchRuntimeEvent &ev)
            {
                ProjectActions::LaunchStandalone();
                return true;
            });
        dispatcher.Dispatch<AppResetLayoutEvent>(
            [this](AppResetLayoutEvent &ev)
            {
                m_Layout->ResetLayout();
                return true;
            });

        dispatcher.Dispatch<EntitySelectedEvent>(
            [this](EntitySelectedEvent &ev)
            {
                m_SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
                m_LastHitMeshIndex = ev.GetMeshIndex();
                return false;
            });

        // Propagate events to all panels
        m_Panels->OnEvent(e);

        if (e.Handled)
            return;

        // Handle actions
        if (m_Actions->OnEvent(e))
            return;

        auto activeScene = Application::Get().GetActiveScene();
        if (e.GetEventType() == EventType::KeyPressed)
        {
            auto &ke = (KeyPressedEvent &)e;
            if (ke.GetKeyCode() == KEY_ESCAPE && m_FullscreenGame)
            {
                m_FullscreenGame = false;
                e.Handled = true;
            }
            activeScene->OnEvent(e);
        }
    }

    CommandHistory &EditorLayer::GetCommandHistory()
    {
        auto &layers = Application::Get().GetLayerStack().GetLayers();
        return ((EditorLayer *)layers.back())->m_CommandHistory;
    }

    SceneState EditorLayer::GetSceneState()
    {
        return s_Instance->m_SceneState;
    }

    EditorLayer &EditorLayer::Get()
    {
        return *s_Instance;
    }

    bool EditorLayer::OnScenePlay(ScenePlayEvent &e)
    {
        CH_CORE_INFO("Scene Play Event");

        m_EditorScene = Application::Get().GetActiveScene();
        if (!m_EditorScene)
            return false;

        std::shared_ptr<Scene> runtimeScene = Scene::Copy(m_EditorScene);
        if (!runtimeScene)
        {
            CH_CORE_ERROR("Failed to clone scene for play mode!");
            return false;
        }

        m_SceneState = SceneState::Play;
        Application::Get().SetActiveScene(runtimeScene);

        // Update panels context
        m_Panels->SetContext(runtimeScene);

        runtimeScene->OnRuntimeStart();

        return false;
    }

    bool EditorLayer::OnSceneStop(SceneStopEvent &e)
    {
        CH_CORE_INFO("Scene Stop Event");
        m_FullscreenGame = false;

        auto runtimeScene = Application::Get().GetActiveScene();
        if (runtimeScene)
            runtimeScene->OnRuntimeStop();

        m_SceneState = SceneState::Edit;
        m_StandaloneActive = false;

        if (m_EditorScene)
        {
            Application::Get().SetActiveScene(m_EditorScene);
            m_Panels->SetContext(m_EditorScene);

            m_EditorScene = nullptr;
        }

        return false;
    }

} // namespace CHEngine
