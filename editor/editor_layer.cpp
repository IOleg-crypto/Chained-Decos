#include "editor_layer.h"
#include "editor_settings.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/core/process_utils.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "raylib.h"
#include "ui/toolbar.h"
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <nfd.h>
#include <rlImGui.h>

#include "panels/console_panel.h"
#include <cstdarg>
#include <cstdio>

namespace CHEngine
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
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

    rlImGuiBeginInitImGui();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(
        AssetManager::ResolvePath("engine:font/lato/Lato-Regular.ttf").string().c_str(), 18.0f);

    static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(
        AssetManager::ResolvePath("engine:font/fa-solid-900.ttf").string().c_str(), 18.0f,
        &icons_config, icons_ranges);

    io.Fonts->AddFontFromFileTTF(
        AssetManager::ResolvePath("engine:font/lato/Lato-Bold.ttf").string().c_str(), 20.0f);
    io.Fonts->AddFontFromFileTTF(
        AssetManager::ResolvePath("engine:font/lato/Lato-Bold.ttf").string().c_str(), 26.0f);

    rlImGuiEndInitImGui();
    SetDarkThemeColors();

    NFD_Init();

    m_EditorCamera.SetPosition({10.0f, 10.0f, 10.0f});
    m_EditorCamera.SetTarget({0.0f, 0.0f, 0.0f});
    m_EditorCamera.SetFOV(90.0f);

    m_ProjectBrowserPanel.SetEventCallback([this](Event &e) { OnEvent(e); });
    m_ViewportPanel.SetEventCallback([this](Event &e) { OnEvent(e); });
    m_SceneHierarchyPanel.SetEventCallback([this](Event &e) { OnEvent(e); });

    m_CommandHistory.SetNotifyCallback(
        [this]()
        {
            // When a command is undone/redone, we might need to refresh panels
            // For now, we just log it, but we could dispatch a SceneChangedEvent
            CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor...");
        });

    // Clear project at start to show project browser
    Project::SetActive(nullptr);

    m_ContentBrowserPanel.SetSceneOpenCallback([this](const std::filesystem::path &path)
                                               { OpenScene(path); });

    CH_CORE_INFO("EditorLayer Attached.");
}

void EditorLayer::OnDetach()
{
    NFD_Quit();
    rlImGuiShutdown();
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (IsKeyPressed(KEY_F11))
    {
        ToggleFullscreen();
    }

    if (m_SceneState == SceneState::Edit)
    {
        m_EditorCamera.OnUpdate(deltaTime);

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
                    SaveScene();
                if (IsKeyPressed(KEY_O))
                    OpenScene();
                if (IsKeyPressed(KEY_N))
                    NewScene();
            }
        }

        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
        {
            Physics::Update(activeScene.get(), deltaTime, m_SceneState == SceneState::Play);
        }
    }
    else if (m_SceneState == SceneState::Play)
    {
        auto activeScene = Application::Get().GetActiveScene();
        if (activeScene)
        {
            Physics::Update(activeScene.get(), deltaTime, true);
            activeScene->OnUpdateRuntime(deltaTime);
        }
    }
}

void EditorLayer::OnRender()
{
    ClearBackground(BLACK);
}

void EditorLayer::OnImGuiRender()
{
    rlImGuiBegin();

    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuizmo::BeginFrame();

    if (Project::GetActive())
    {
        UI_DrawDockSpace();
        UI_DrawPanels();
        ImGui::End(); // End DockSpace window (started in UI_DrawDockSpace)
    }
    else
    {
        m_ProjectBrowserPanel.OnImGuiRender();
    }

    rlImGuiEnd();
}

void EditorLayer::UI_DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project", "Ctrl+Shift+N"))
                NewProject();
            if (ImGui::MenuItem("Open Project", "Ctrl+O"))
                OpenProject();
            if (ImGui::MenuItem("Save Project"))
                SaveProject();
            if (ImGui::MenuItem("Close Project"))
                Project::SetActive(nullptr);

            ImGui::Separator();

            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                NewScene();
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                SaveScene();
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                SaveSceneAs();
            if (ImGui::MenuItem("Load Scene", "Ctrl+L"))
                OpenScene();

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
                Application::Shutdown();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                m_CommandHistory.Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y"))
                m_CommandHistory.Redo();

            ImGui::Separator();

            if (ImGui::MenuItem("Select", "Q", m_CurrentTool == GizmoType::NONE))
                m_CurrentTool = GizmoType::NONE;
            if (ImGui::MenuItem("Translate", "W", m_CurrentTool == GizmoType::TRANSLATE))
                m_CurrentTool = GizmoType::TRANSLATE;
            if (ImGui::MenuItem("Rotate", "E", m_CurrentTool == GizmoType::ROTATE))
                m_CurrentTool = GizmoType::ROTATE;
            if (ImGui::MenuItem("Scale", "R", m_CurrentTool == GizmoType::SCALE))
                m_CurrentTool = GizmoType::SCALE;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Viewport", nullptr, true);
            ImGui::MenuItem("Scene Hierarchy", nullptr, true);
            ImGui::MenuItem("Inspector", nullptr, true);
            ImGui::MenuItem("Skybox", nullptr, &m_EnvironmentPanel.IsOpen());
            ImGui::MenuItem("Content Browser", nullptr, &m_ShowContentBrowser);
            ImGui::MenuItem("Console", nullptr, true);
            ImGui::Separator();
            if (ImGui::MenuItem("Fullscreen", "F11", IsWindowFullscreen()))
                ToggleFullscreen();
            if (ImGui::MenuItem("Reset UI Layout"))
                ResetLayout();

            ImGui::EndMenu();
        }

        // Scripting menu removed (C# remnants)
        ImGui::EndMenuBar();
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

    UI_DrawMenuBar();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void EditorLayer::UI_DrawPanels()
{
    UI::DrawToolbar(m_SceneState == SceneState::Play, [this](Event &e) { OnEvent(e); });

    bool bIsEdit = m_SceneState == SceneState::Edit;
    m_ViewportPanel.OnImGuiRender(Application::Get().GetActiveScene().get(), GetActiveCamera(),
                                  m_SceneHierarchyPanel.GetSelectedEntity(), m_CurrentTool, m_Gizmo,
                                  &m_DebugRenderFlags, bIsEdit);

    m_SceneHierarchyPanel.OnImGuiRender(!bIsEdit);
    if (m_ShowContentBrowser)
    {
        m_ContentBrowserPanel.OnImGuiRender(&m_ShowContentBrowser, !bIsEdit);
    }
    m_ConsolePanel.OnImGuiRender(!bIsEdit);

    auto activeScene = Application::Get().GetActiveScene();
    m_EnvironmentPanel.OnImGuiRender(activeScene.get(), !bIsEdit, &m_DebugRenderFlags);
    m_InspectorPanel.OnImGuiRender(activeScene.get(), m_SceneHierarchyPanel.GetSelectedEntity(),
                                   !bIsEdit);
}

void EditorLayer::NewProject()
{
    nfdchar_t *savePath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, NULL, "Untitled.chproject");
    if (result == NFD_OKAY)
    {
        NewProject(std::filesystem::path(savePath).stem().string(), savePath);
        NFD_FreePath(savePath);
    }
}

void EditorLayer::NewProject(const std::string &name, const std::string &path)
{
    CH_CORE_INFO("EditorLayer: NewProject request - Name: '{0}', Raw Path: '{1}'", name, path);

    std::filesystem::path rootPath = std::filesystem::path(path).lexically_normal();
    if (rootPath.has_filename() && rootPath.filename() == ".")
        rootPath = rootPath.parent_path();

    std::filesystem::path projectDir;

    // Robust case-insensitive comparison for Windows
    std::string folderName = rootPath.filename().string();
    std::string projName = name;

    std::string folderLower = folderName;
    std::transform(folderLower.begin(), folderLower.end(), folderLower.begin(), ::tolower);
    std::string projLower = projName;
    std::transform(projLower.begin(), projLower.end(), projLower.begin(), ::tolower);

    if (!folderLower.empty() && folderLower == projLower)
    {
        projectDir = rootPath;
        CH_CORE_INFO(
            "EditorLayer: Selected folder '{0}' matches project name '{1}'. Using direct path: {2}",
            folderName, name, projectDir.string());
    }
    else
    {
        projectDir = rootPath / name;
        CH_CORE_INFO("EditorLayer: Creating dedicated subfolder: {0}", projectDir.string());
    }

    CH_CORE_INFO("EditorLayer: Resolved final project directory: {0}", projectDir.string());

    // Ensure the project directory exists
    try
    {
        if (!std::filesystem::exists(projectDir))
        {
            if (std::filesystem::create_directories(projectDir))
                CH_CORE_INFO("Created project directory: {0}", projectDir.string());
            else
                CH_CORE_ERROR("Failed to create project directory: {0}", projectDir.string());
        }
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Exception while creating directory: {0}", e.what());
        return;
    }

    std::filesystem::path projectFilePath = projectDir / (name + ".chproject");
    CH_CORE_INFO("Generated project file path: {0}", projectFilePath.string());

    Ref<Project> newProject = CreateRef<Project>();
    newProject->SetName(name);
    newProject->SetProjectDirectory(projectDir);

    ProjectSerializer serializer(newProject);
    if (serializer.Serialize(projectFilePath))
    {
        CH_CORE_INFO("Project serialization successful: {0}", projectFilePath.string());
        Project::SetActive(newProject);
        EditorSettings::SetLastProjectPath(projectFilePath.string());
        EditorSettings::Save();

        std::filesystem::create_directories(projectDir / "assets/scenes");
        m_ContentBrowserPanel.SetRootDirectory(newProject->GetAssetDirectory());

        NewScene();
    }
    else
    {
        CH_CORE_ERROR("Failed to serialize project file: {0}", projectFilePath.string());
    }
}

void EditorLayer::OpenProject()
{
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        OpenProject(outPath);
        NFD_FreePath(outPath);
    }
}

extern void RegisterGameScripts(); // From ChainedDecosGame library

void EditorLayer::OpenProject(const std::filesystem::path &path)
{
    CH_CORE_INFO("Opening project: {0}", path.string());
    Ref<Project> project = CreateRef<Project>();
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        Project::SetActive(project);
        EditorSettings::SetLastProjectPath(path.string());
        EditorSettings::Save();

        m_CurrentTool = GizmoType::TRANSLATE;
        CH_CORE_INFO("Project Opened successfully: {0}", path.string());

        // Register game scripts from the linked library
        RegisterGameScripts();

        m_ContentBrowserPanel.SetRootDirectory(project->GetAssetDirectory());

        auto &config = project->GetConfig();
        if (!config.ActiveScenePath.empty())
        {
            std::filesystem::path scenePath =
                project->GetConfig().ProjectDirectory / config.ActiveScenePath;
            CH_CORE_INFO("Loading active scene: {0}", scenePath.string());
            OpenScene(scenePath);
        }
        else
        {
            CH_CORE_INFO("No active scene found in project file, creating new scene.");
            NewScene();
        }
    }
    else
    {
        CH_CORE_ERROR("Failed to deserialize project file: {0}", path.string());
    }
}

void EditorLayer::SaveProject()
{
    auto project = Project::GetActive();
    if (project)
    {
        SaveScene();
        ProjectSerializer serializer(project);
        std::filesystem::path projectFile =
            project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject");

        if (serializer.Serialize(projectFile))
        {
            CH_CORE_INFO("Project Saved successfully: {0}", projectFile.string());
        }
        else
        {
            CH_CORE_ERROR("Failed to save project: {0}", projectFile.string());
        }
    }
    else
    {
        CH_CORE_WARN("No active project to save!");
    }
}

void EditorLayer::NewScene()
{
    auto newScene = CreateRef<Scene>();
    newScene->CreateEntity("Default Entity");
    Application::Get().SetActiveScene(newScene);
    m_SceneHierarchyPanel.SetContext(newScene);
}

void EditorLayer::OpenScene()
{
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        OpenScene(outPath);
        NFD_FreePath(outPath);
    }
}

void EditorLayer::OpenScene(const std::filesystem::path &path)
{
    Application::Get().LoadScene(path.string());
    auto activeScene = Application::Get().GetActiveScene();
    if (activeScene)
    {
        m_SceneHierarchyPanel.SetContext(activeScene);

        auto project = Project::GetActive();
        if (project)
        {
            std::error_code ec;
            auto relPath =
                std::filesystem::relative(path, project->GetConfig().ProjectDirectory, ec);
            if (!ec)
                project->SetActiveScenePath(relPath);
        }
    }
}

void EditorLayer::SaveScene()
{
    auto project = Project::GetActive();
    if (project && !project->GetConfig().ActiveScenePath.empty())
    {
        std::filesystem::path fullPath =
            project->GetConfig().ProjectDirectory / project->GetConfig().ActiveScenePath;

        auto activeScene = Application::Get().GetActiveScene();
        SceneSerializer serializer(activeScene.get());
        if (serializer.Serialize(fullPath.string()))
        {
            CH_CORE_INFO("Scene Saved successfully: {0}", fullPath.string());
        }
        else
        {
            CH_CORE_ERROR("Failed to save scene: {0}", fullPath.string());
        }
    }
    else
    {
        CH_CORE_INFO("No active scene path, calling Save Scene As...");
        SaveSceneAs();
    }
}

void EditorLayer::SaveSceneAs()
{
    nfdchar_t *savePath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, NULL, "Untitled.chscene");
    if (result == NFD_OKAY)
    {
        auto activeScene = Application::Get().GetActiveScene();
        SceneSerializer serializer(activeScene.get());
        if (serializer.Serialize(savePath))
        {
            auto project = Project::GetActive();
            if (project)
            {
                std::error_code ec;
                auto relPath =
                    std::filesystem::relative(savePath, project->GetConfig().ProjectDirectory, ec);
                if (!ec)
                    project->SetActiveScenePath(relPath);
            }
            CH_CORE_INFO("Scene Saved As: {0}", savePath);
        }
        else
        {
            CH_CORE_ERROR("Failed to save scene to: {0}", savePath);
        }
        NFD_FreePath(savePath);
    }
}

void EditorLayer::ResetLayout()
{
    m_ShowContentBrowser = true;
    m_EnvironmentPanel.IsOpen() = true;

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
    ImGui::DockBuilderDockWindow("Skybox", dock_right);
    ImGui::DockBuilderDockWindow("Content Browser", dock_down);
    ImGui::DockBuilderDockWindow("Console", dock_down);

    ImGui::DockBuilderFinish(dockspace_id);
}

void EditorLayer::OnEvent(Event &e)
{
    auto activeScene = Application::Get().GetActiveScene();
    if (activeScene)
        activeScene->OnEvent(e);

    if (e.Handled)
        return;

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (m_SceneState == SceneState::Play && ev.GetKeyCode() == KEY_ESCAPE)
            {
                OnSceneStop();
                return true;
            }

            if (m_SceneState == SceneState::Edit && !Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                switch (ev.GetKeyCode())
                {
                case KEY_Q:
                    m_CurrentTool = GizmoType::NONE;
                    return true;
                case KEY_W:
                    m_CurrentTool = GizmoType::TRANSLATE;
                    return true;
                case KEY_E:
                    m_CurrentTool = GizmoType::ROTATE;
                    return true;
                case KEY_R:
                    m_CurrentTool = GizmoType::SCALE;
                    return true;
                }
            }

            return false;
        });

    dispatcher.Dispatch<ScenePlayEvent>(
        [this](ScenePlayEvent &ev)
        {
            OnScenePlay();
            return true;
        });

    dispatcher.Dispatch<SceneStopEvent>(
        [this](SceneStopEvent &ev)
        {
            OnSceneStop();
            return true;
        });

    dispatcher.Dispatch<AppLaunchRuntimeEvent>(
        [this](AppLaunchRuntimeEvent &ev)
        {
            LaunchStandalone();
            return true;
        });

    dispatcher.Dispatch<ProjectOpenedEvent>(
        [this](ProjectOpenedEvent &ev)
        {
            OpenProject(ev.GetPath());
            return true;
        });

    dispatcher.Dispatch<ProjectCreatedEvent>(
        [this](ProjectCreatedEvent &ev)
        {
            NewProject(ev.GetProjectName(), ev.GetPath());
            return true;
        });

    dispatcher.Dispatch<EntitySelectedEvent>(
        [this](EntitySelectedEvent &ev)
        {
            m_SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
            m_LastHitMeshIndex = ev.GetMeshIndex();
            m_SceneHierarchyPanel.SetSelectedEntity(m_SelectedEntity);
            m_InspectorPanel.SetSelectedMeshIndex(m_LastHitMeshIndex);
            return true;
        });
}
void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;

    auto activeScene = Application::Get().GetActiveScene();
    auto spawnView = activeScene->GetRegistry().view<SpawnComponent, TransformComponent>();
    for (auto entity : spawnView)
    {
        auto &spawn = spawnView.get<SpawnComponent>(entity);
        auto &transform = spawnView.get<TransformComponent>(entity);
        spawn.RenderSpawnZoneInScene = false;
        spawn.SpawnPoint = transform.Translation;
    }
    DisableCursor();
    CH_CORE_INFO("Scene Started.");
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;

    auto activeScene = Application::Get().GetActiveScene();
    auto spawnView = activeScene->GetRegistry().view<SpawnComponent>();
    for (auto entity : spawnView)
    {
        auto &spawn = spawnView.get<SpawnComponent>(entity);
        spawn.RenderSpawnZoneInScene = true;
    }

    EnableCursor();
    CH_CORE_INFO("Scene Stopped.");
}

void EditorLayer::LaunchStandalone()
{
    SaveProject();
    auto project = Project::GetActive();
    if (project)
    {
        std::filesystem::path projectFile =
            project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject");

        // Robust runtime detection
        std::filesystem::path runtimePath;
        std::filesystem::path exePath = ProcessUtils::GetExecutablePath();
        std::vector<std::filesystem::path> searchPaths = {
            exePath.parent_path(),               // editor/
            exePath.parent_path().parent_path(), // build/
            std::filesystem::current_path(),     // cwd
        };

        for (const auto &base : searchPaths)
        {
            if (std::filesystem::exists(base / "bin/ChainedRuntime.exe"))
            {
                runtimePath = base / "bin/ChainedRuntime.exe";
                break;
            }
            if (std::filesystem::exists(base / "ChainedRuntime.exe"))
            {
                runtimePath = base / "ChainedRuntime.exe";
                break;
            }
        }

        if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
        {
            CH_CORE_ERROR("Could not find ChainedRuntime.exe! Checked: {}",
                          searchPaths[0].string());
            return;
        }

        std::string command =
            std::format("\"{}\" \"{}\"", runtimePath.string(), projectFile.string());
        CH_CORE_INFO("Launching Standalone: {}", command);

        if (!ProcessUtils::LaunchProcess(command))
        {
            CH_CORE_ERROR("Failed to launch standalone runtime: {}", command);
        }
    }
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
        return m_EditorCamera.GetRaylibCamera();

    auto activeScene = Application::Get().GetActiveScene();
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
    return m_EditorCamera.GetRaylibCamera();
}
} // namespace CHEngine
