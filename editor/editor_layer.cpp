#include "editor_layer.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "ui/editor_gui.h"

// Removed redundant include: engine/core/process_utils.h
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
// Removed redundant include: engine/graphics/asset_manager.h
// Removed redundant include: engine/graphics/render.h
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"

#include "cstdarg"
#include "extras/iconsfontawesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "nfd.h"
#include "panels/component_ui.h"
#include "panels/console_panel.h"
#include "raylib.h"

namespace CHEngine
{
EditorLayer *EditorLayer::s_Instance = nullptr;
ImVec2 EditorLayer::s_ViewportSize = {1280, 720};

EditorLayer::EditorLayer() : Layer("EditorLayer")
{
    s_Instance = this;
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

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    EditorUI::GUI::SetDarkThemeColors();

    NFD_Init();
    ComponentUI::Init();

    // Register Panels
    AddPanel<ViewportPanel>()->SetDebugFlags(&m_DebugRenderFlags);
    AddPanel<SceneHierarchyPanel>();
    AddPanel<InspectorPanel>();
    AddPanel<ContentBrowserPanel>()->SetSceneOpenCallback([](const std::filesystem::path &path)
                                                          { SceneActions::Open(path); });
    AddPanel<ConsolePanel>();
    AddPanel<EnvironmentPanel>()->SetDebugFlags(&m_DebugRenderFlags);
    AddPanel<ProfilerPanel>();
    AddPanel<ProjectBrowserPanel>();
    AddPanel<ProjectSettingsPanel>();

    m_CommandHistory.SetNotifyCallback(
        [this]() { CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor..."); });

    // Auto-load last project/scene
    const auto &config = Editor::Get().GetEditorConfig();
    if (!config.LastProjectPath.empty() && std::filesystem::exists(config.LastProjectPath))
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
        // Clear project at start to show project browser if no auto-load
        Project::SetActive(nullptr);
    }

    CH_CORE_INFO("EditorLayer Attached with {} modular panels.", m_Panels.size());
}

void EditorLayer::OnDetach()
{
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
        if (!Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (control)
            {
                if (IsKeyPressed(KEY_Z))
                    m_CommandHistory.Undo();
                if (IsKeyPressed(KEY_Y))
                    m_CommandHistory.Redo();
                if (IsKeyPressed(KEY_S))
                {
                    if (activeScene->GetScenePath().empty())
                    {
                        // If scene is unsaved, treat Ctrl+S as Save As
                        SceneActions::SaveAs();
                    }
                    else
                    {
                        SceneActions::Save();
                    }
                }
                if (Input::IsKeyPressed(KeyboardKey::KEY_F5))
                    ProjectActions::LaunchStandalone();
                if (IsKeyPressed(KEY_N))
                    SceneActions::New();
            }
        }

        for (auto &panel : m_Panels)
            panel->OnUpdate(deltaTime);
    }

    // Standalone rendering logic removed in favor of ImGui Viewports
}

void EditorLayer::OnRender()
{
    ClearBackground(BLACK);
}

void EditorLayer::OnImGuiRender()
{
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuizmo::BeginFrame();

    if (Project::GetActive())
    {
        if (m_FullscreenGame)
        {
            // Fullscreen mode: only render viewport
            auto viewportPanel = GetPanel<ViewportPanel>();
            if (viewportPanel)
                viewportPanel->OnImGuiRender(true);
        }
        else
        {
            // Normal editor mode
            DrawDockSpace();
            DrawPanels();
            DrawScriptUI();

            ImGui::End(); // End DockSpace window (started in DrawDockSpace)
        }
    }
    else
    {
        if (auto projectBrowser = GetPanel<ProjectBrowserPanel>())
            projectBrowser->OnImGuiRender();
    }
}

void EditorLayer::DrawDockSpace()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Draw Menu Bar
    EditorUI::MenuBarState menuState;
    menuState.IsPlaying = (m_SceneState == SceneState::Play);
    menuState.Panels = &m_Panels;
    EditorUI::GUI::DrawMenuBar(menuState, CH_BIND_EVENT_FN(EditorLayer::OnEvent));

    // Draw Toolbar
    EditorUI::GUI::DrawToolbar(m_SceneState == SceneState::Play,
                               CH_BIND_EVENT_FN(EditorLayer::OnEvent));

    // Draw Central UI
    if (!Project::GetActive())
    {
        EditorUI::GUI::DrawProjectSelector(
            Project::GetActive() != nullptr, {0}, []() { ProjectActions::Open(); },
            []() { ProjectActions::Open(); }, []() { Application::Get().Close(); });
    }
    else
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        bool readOnly = m_SceneState == SceneState::Play;
        for (auto &panel : m_Panels)
        {
            // Project Browser is handled separately
            if (panel->GetName() == "Project Browser")
                continue;

            panel->OnImGuiRender(readOnly);
        }
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

void EditorLayer::DrawPanels()
{
    EditorUI::GUI::DrawToolbar(m_SceneState == SceneState::Play, [this](Event &e) { OnEvent(e); });
    bool readOnly = m_SceneState == SceneState::Play;
    for (auto &panel : m_Panels)
    {
        // Project Browser is handled separately
        if (panel->GetName() == "Project Browser")
            continue;

        panel->OnImGuiRender(readOnly);
    }
}

void EditorLayer::ResetLayout()
{
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace |
                                                ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_right =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_left =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_down =
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

    ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
    ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left);
    ImGui::DockBuilderDockWindow("Inspector", dock_right);
    ImGui::DockBuilderDockWindow("Environment", dock_right);
    ImGui::DockBuilderDockWindow("Profiler", dock_right);
    ImGui::DockBuilderDockWindow("Content Browser", dock_down);
    ImGui::DockBuilderDockWindow("Console", dock_down);

    ImGui::DockBuilderFinish(dockspace_id);
}

bool EditorLayer::OnProjectOpened(ProjectOpenedEvent &e)
{
    CH_CORE_INFO("EditorLayer: Handling ProjectOpenedEvent - {}", e.GetPath());

    // Project is already loaded by EditorUtils::ProjectActions::OpenProject
    // or by the ProjectBrowserPanel. Just update UI panels.

    auto project = Project::GetActive();
    if (project)
    {
        if (auto contentBrowser = GetPanel<ContentBrowserPanel>())
            contentBrowser->SetRootDirectory(Project::GetAssetDirectory());
    }
    return false;
}

bool EditorLayer::OnSceneOpened(SceneOpenedEvent &e)
{
    auto activeScene = Application::Get().GetActiveScene();
    for (auto &panel : m_Panels)
        panel->SetContext(activeScene);

    m_SelectedEntity = {};

    // Sync project path
    auto project = Project::GetActive();
    if (project && !e.GetPath().empty())
    {
        project->SetActiveScenePath(
            std::filesystem::relative(e.GetPath(), project->GetProjectDirectory()));
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
            ResetLayout();
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
    for (auto &panel : m_Panels)
        panel->OnEvent(e);

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

    if (e.Handled)
        return;

    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (m_SceneState == SceneState::Play && ev.GetKeyCode() == KEY_ESCAPE)
            {
                SceneStopEvent stopEv;
                OnEvent(stopEv);
                return true;
            }

            // Shortcuts
            bool control = Input::IsKeyDown(KeyboardKey::KEY_LEFT_CONTROL) ||
                           Input::IsKeyDown(KeyboardKey::KEY_RIGHT_CONTROL);
            bool shift = Input::IsKeyDown(KeyboardKey::KEY_LEFT_SHIFT) ||
                         Input::IsKeyDown(KeyboardKey::KEY_RIGHT_SHIFT);

            if (control)
            {
                switch (ev.GetKeyCode())
                {
                case KeyboardKey::KEY_N:
                    SceneActions::New();
                    return true;
                case KeyboardKey::KEY_O:
                    SceneActions::Open();
                    return true;
                case KeyboardKey::KEY_S:
                    if (shift)
                        SceneActions::SaveAs();
                    else
                        SceneActions::Save();
                    return true;
                }
            }

            if (ev.GetKeyCode() == KeyboardKey::KEY_F5)
            {
                ProjectActions::LaunchStandalone();
                return true;
            }

            return false;
        });
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
    for (auto &panel : m_Panels)
        panel->SetContext(runtimeScene);

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
        for (auto &panel : m_Panels)
            panel->SetContext(m_EditorScene);

        m_EditorScene = nullptr;
    }

    return false;
}

} // namespace CHEngine
