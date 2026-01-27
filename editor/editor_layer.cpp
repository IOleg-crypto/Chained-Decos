#include "editor_layer.h"
#include "editor_settings.h"
#include "editor_utils.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/core/process_utils.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/render/asset_manager.h"
#include "engine/render/render.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"
#include "engine/ui/imgui_raylib_ui.h"
#include "raylib.h"
#include "ui/menu_bar.h"
#include "ui/toolbar.h"
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>

// GLFW and OpenGL for context management
#define GLFW_INCLUDE_NONE
#include "external/glfw/include/GLFW/glfw3.h"
#include <rlgl.h>

#include "panels/component_ui.h"
#include "panels/console_panel.h"
#include <cstdarg>
#include <cstdio>

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

    SetDarkThemeColors();

    NFD_Init();
    ComponentUI::Init();

    // Register Panels
    AddPanel<ViewportPanel>()->SetDebugFlags(&m_DebugRenderFlags);
    AddPanel<SceneHierarchyPanel>();
    AddPanel<InspectorPanel>();
    AddPanel<ContentBrowserPanel>()->SetSceneOpenCallback([](const std::filesystem::path &path)
                                                          { SceneUtils::OpenScene(path); });
    AddPanel<ConsolePanel>();
    AddPanel<EnvironmentPanel>()->SetDebugFlags(&m_DebugRenderFlags);
    AddPanel<ProfilerPanel>();
    AddPanel<ProjectBrowserPanel>();
    AddPanel<ProjectSettingsPanel>();

    m_CommandHistory.SetNotifyCallback(
        [this]() { CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor..."); });

    // Auto-load last project/scene
    const auto &settings = EditorSettings::Get();
    if (!settings.LastProjectPath.empty() && std::filesystem::exists(settings.LastProjectPath))
    {
        CH_CORE_INFO("Auto-loading last project: {}", settings.LastProjectPath);
        ProjectUtils::OpenProject(settings.LastProjectPath);

        if (!settings.LastScenePath.empty() && std::filesystem::exists(settings.LastScenePath))
        {
            CH_CORE_INFO("Auto-loading last scene: {}", settings.LastScenePath);
            SceneUtils::OpenScene(settings.LastScenePath);
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

    if (IsKeyPressed(KEY_F11))
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
                        SceneUtils::SaveSceneAs();
                    }
                    else
                    {
                        SceneUtils::SaveScene();
                    }
                }
                if (IsKeyPressed(KEY_O))
                    SceneUtils::OpenScene();
                if (IsKeyPressed(KEY_N))
                    SceneUtils::NewScene();
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
            UI_DrawDockSpace();
            UI_DrawPanels();
            UI_DrawScriptUI();

            ImGui::End(); // End DockSpace window (started in UI_DrawDockSpace)
        }
    }
    else
    {
        if (auto projectBrowser = GetPanel<ProjectBrowserPanel>())
            projectBrowser->OnImGuiRender();
    }
}

void EditorLayer::UI_DrawDockSpace()
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

    UI::MenuBarState menuBarState;
    menuBarState.IsPlaying = m_SceneState == SceneState::Play;
    menuBarState.Panels = &m_Panels;

    UI::DrawMenuBar(menuBarState, [this](Event &e) { OnEvent(e); });

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void EditorLayer::UI_DrawScriptUI()
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

void EditorLayer::UI_DrawPanels()
{
    UI::DrawToolbar(m_SceneState == SceneState::Play, [this](Event &e) { OnEvent(e); });

    bool readOnly = m_SceneState == SceneState::Play;
    for (auto &panel : m_Panels)
    {
        // Project Browser is handled separately
        if (panel->GetName() == "Project Browser")
            continue;

        panel->OnImGuiRender(readOnly);
    }
}

void EditorLayer::LaunchStandalone()
{
    // Auto-save project and scene
    ProjectUtils::SaveProject();

    auto project = Project::GetActive();
    if (project)
    {
        std::filesystem::path projectFile =
            project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
        std::string projectPathStr = std::filesystem::absolute(projectFile).string();

        // Robust runtime discovery
        std::filesystem::path runtimePath;
        Configuration buildConfig = project->GetConfig().BuildConfig;
        std::string configStr = (buildConfig == Configuration::Release) ? "Release" : "Debug";

        CH_CORE_INFO("LaunchStandalone: Searching for {0} configuration runtime...", configStr);

        std::filesystem::path exePath = ProcessUtils::GetExecutablePath();
        std::filesystem::path buildDir = exePath.parent_path().parent_path(); // Likely 'build/'
        std::filesystem::path cwd = std::filesystem::current_path();

        std::vector<std::filesystem::path> searchBases = {
            buildDir,    buildDir / "bin",  buildDir / "runtime", cwd,
            cwd / "bin", cwd / "build/bin", exePath.parent_path()};

        std::vector<std::string> targetNames = {project->GetConfig().Name + ".exe",
                                                "ChainedRuntime.exe", "Runtime.exe"};

        for (const auto &base : searchBases)
        {
            for (const auto &name : targetNames)
            {
                // 1. Try nested (multi-config): base/config/name
                std::filesystem::path nested = base / configStr / name;
                if (std::filesystem::exists(nested))
                {
                    runtimePath = nested;
                    break;
                }

                // 2. Try flat (single-config): base/name
                std::filesystem::path flat = base / name;
                if (std::filesystem::exists(flat))
                {
                    runtimePath = flat;
                    break;
                }
            }
            if (!runtimePath.empty())
                break;
        }

        if (runtimePath.empty())
        {
            CH_CORE_ERROR("LaunchStandalone: Failed to find runtime executable.");
            CH_CORE_INFO("Searched locations for '{0}' or 'ChainedRuntime.exe':",
                         project->GetConfig().Name);
            for (const auto &base : searchBases)
                CH_CORE_INFO("  - {0}", base.string());
            return;
        }

        if (!std::filesystem::exists(projectPathStr))
        {
            CH_CORE_ERROR("LaunchStandalone: Project file not found: {0}", projectPathStr);
            return;
        }

        std::string command = std::format("\"{}\" \"{}\"", runtimePath.string(), projectPathStr);
        CH_CORE_INFO("Launching Standalone: {0}", command);

        if (!ProcessUtils::LaunchProcess(command))
        {
            CH_CORE_ERROR("LaunchStandalone: Failed to launch process: {0}", command);
        }
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

    // Project is already loaded by EditorUtils::ProjectUtils::OpenProject
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
        ProjectUtils::SaveProject();
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
            ProjectUtils::NewProject(ev.GetProjectName(), ev.GetPath());
            return true;
        });
    dispatcher.Dispatch<SceneOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneOpened));
    dispatcher.Dispatch<ScenePlayEvent>(CH_BIND_EVENT_FN(EditorLayer::OnScenePlay));
    dispatcher.Dispatch<SceneStopEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneStop));
    dispatcher.Dispatch<AppLaunchRuntimeEvent>(
        [this](AppLaunchRuntimeEvent &ev)
        {
            LaunchStandalone();
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
            return false;
        });
}

void EditorLayer::SetDarkThemeColors()
{
    auto &colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
}

CommandHistory &EditorLayer::GetCommandHistory()
{
    auto &layers = Application::Get().GetLayerStack().GetLayers();
    return ((EditorLayer *)layers.back())->m_CommandHistory;
}

Camera3D EditorLayer::GetActiveCamera()
{
    if (m_SceneState == SceneState::Edit)
    {
        if (auto viewport = GetPanel<ViewportPanel>())
            return viewport->GetCamera().GetRaylibCamera();
    }

    auto activeScene = Application::Get().GetActiveScene();
    if (!activeScene)
    {
        Camera3D camera = {0};
        camera.position = {10.0f, 10.0f, 10.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

    auto view = activeScene->GetRegistry().view<PlayerComponent, TransformComponent>();
    if (view.begin() != view.end())
    {
        auto entity = *view.begin();
        auto &transform = view.get<TransformComponent>(entity);
        auto &player = view.get<PlayerComponent>(entity);

        Vector3 target = transform.Translation;
        target.y += 1.0f;
        float yawRad = player.CameraYaw * DEG2RAD;
        float pitchRad = player.CameraPitch * DEG2RAD;
        Vector3 offset;
        offset.x = player.CameraDistance * cosf(pitchRad) * sinf(yawRad);
        offset.y = player.CameraDistance * sinf(pitchRad);
        offset.z = player.CameraDistance * cosf(pitchRad) * cosf(yawRad);
        Camera3D camera = {0};
        camera.position = Vector3Add(target, offset);
        camera.target = target;
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 90.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        return camera;
    }

    // Default fallback
    Camera3D camera = {0};
    camera.position = {10.0f, 10.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

SceneState EditorLayer::GetSceneState()
{
    return s_Instance->m_SceneState;
}

EditorLayer &EditorLayer::Get()
{
    return *s_Instance;
}

} // namespace CHEngine
