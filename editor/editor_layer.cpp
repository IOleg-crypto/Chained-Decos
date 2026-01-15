#include "editor_layer.h"
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

    m_ActiveScene = CreateRef<Scene>();
    m_ActiveScene->CreateEntity("Default Entity");
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);

    m_ProjectBrowserPanel.SetEventCallback([this](Event &e) { OnEvent(e); });
    m_ViewportPanel.SetEventCallback([this](Event &e) { OnEvent(e); });
    m_SceneHierarchyPanel.SetEventCallback([this](Event &e) { OnEvent(e); });

    if (Project::GetActive())
    {
        auto projectDir = Project::GetProjectDirectory();
        m_ContentBrowserPanel.SetRootDirectory(projectDir);

        std::filesystem::path scenesDir = projectDir / "scenes";
        if (std::filesystem::exists(scenesDir))
            m_ContentBrowserPanel.SetRootDirectory(scenesDir);
    }

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

        if (m_ActiveScene)
        {
            Physics::Update(m_ActiveScene.get(), deltaTime, m_SceneState == SceneState::Play);

            if (m_SceneState == SceneState::Edit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                !ImGui::GetIO().WantCaptureMouse)
            {
                Ray ray = GetMouseRay(GetMousePosition(), m_EditorCamera.GetRaylibCamera());
                RaycastResult result = Physics::Raycast(m_ActiveScene.get(), ray);
                if (result.Hit)
                {
                    m_SceneHierarchyPanel.SetSelectedEntity(
                        Entity{result.Entity, m_ActiveScene.get()});
                }
                else
                {
                    m_SceneHierarchyPanel.SetSelectedEntity({});
                }
            }
        }
    }
    else if (m_SceneState == SceneState::Play)
    {
        if (m_ActiveScene)
        {
            Physics::Update(m_ActiveScene.get(), deltaTime, true);
            m_ActiveScene->OnUpdateRuntime(deltaTime);
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

    UI_DrawDockSpace();

    if (Project::GetActive())
    {
        UI_DrawPanels();
    }
    else
    {
        m_ProjectBrowserPanel.OnImGuiRender();
    }

    ImGui::End();

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

            if (ImGui::MenuItem("Select", "Q", m_CurrentTool == GizmoType::SELECT))
                m_CurrentTool = GizmoType::SELECT;
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
    Entity picked = m_ViewportPanel.OnImGuiRender(
        m_ActiveScene.get(), GetActiveCamera(), m_SceneHierarchyPanel.GetSelectedEntity(),
        m_CurrentTool, m_Gizmo, &m_DebugRenderFlags, bIsEdit);
    if (picked)
    {
        m_SceneHierarchyPanel.SetSelectedEntity(picked);
    }
    m_SceneHierarchyPanel.OnImGuiRender(!bIsEdit);
    if (m_ShowContentBrowser)
    {
        m_ContentBrowserPanel.OnImGuiRender(&m_ShowContentBrowser, !bIsEdit);
    }
    m_ConsolePanel.OnImGuiRender(!bIsEdit);

    m_EnvironmentPanel.OnImGuiRender(m_ActiveScene.get(), !bIsEdit, &m_DebugRenderFlags);
    m_InspectorPanel.OnImGuiRender(m_ActiveScene.get(), m_SceneHierarchyPanel.GetSelectedEntity(),
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
    Ref<Project> newProject = CreateRef<Project>();
    newProject->SetName(name);

    std::filesystem::path projectFilePath = path;
    std::filesystem::path projectDir = projectFilePath.parent_path();

    newProject->SetProjectDirectory(projectDir);

    ProjectSerializer serializer(newProject);
    serializer.Serialize(projectFilePath);
    Project::SetActive(newProject);
    CH_CORE_INFO("Project Created: %s", projectFilePath.string().c_str());

    std::filesystem::create_directories(projectDir / "assets/scenes");
    m_ContentBrowserPanel.SetRootDirectory(newProject->GetAssetDirectory());

    NewScene();
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

void EditorLayer::OpenProject(const std::filesystem::path &path)
{
    Ref<Project> project = CreateRef<Project>();
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        Project::SetActive(project);
        CH_CORE_INFO("Project Opened: %s", path.string().c_str());
        m_ContentBrowserPanel.SetRootDirectory(project->GetAssetDirectory());

        auto &config = project->GetConfig();
        if (!config.ActiveScenePath.empty())
            OpenScene(project->GetConfig().ProjectDirectory / config.ActiveScenePath);
        else
            NewScene();
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
    m_ActiveScene = CreateRef<Scene>();
    m_ActiveScene->CreateEntity("Default Entity");
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
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
    m_ActiveScene = CreateRef<Scene>();
    SceneSerializer serializer(m_ActiveScene.get());
    if (serializer.Deserialize(path.string()))
    {
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

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

        SceneSerializer serializer(m_ActiveScene.get());
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
        SceneSerializer serializer(m_ActiveScene.get());
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
    if (m_ActiveScene)
        m_ActiveScene->OnEvent(e);

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
                    m_CurrentTool = GizmoType::SELECT;
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
            m_SceneHierarchyPanel.SetSelectedEntity(m_SelectedEntity);
            return true;
        });
}
void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;

    auto spawnView = m_ActiveScene->GetRegistry().view<SpawnComponent, TransformComponent>();
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

    auto spawnView = m_ActiveScene->GetRegistry().view<SpawnComponent>();
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

        // Try to find the runtime by searching up from the editor location
        std::filesystem::path editorPath = ProcessUtils::GetExecutablePath();
        std::filesystem::path searchPath = editorPath.parent_path();
        std::filesystem::path runtimePath;

        // Search up to 3 levels up for a 'bin' directory or the executable itself
        for (int i = 0; i < 3; ++i)
        {
            if (std::filesystem::exists(searchPath / "bin" / "ChainedRuntime.exe"))
            {
                runtimePath = searchPath / "bin" / "ChainedRuntime.exe";
                break;
            }
            if (std::filesystem::exists(searchPath / "ChainedRuntime.exe"))
            {
                runtimePath = searchPath / "ChainedRuntime.exe";
                break;
            }
            searchPath = searchPath.parent_path();
        }

        if (runtimePath.empty())
        {
            // Absolute fallback: current working directory / bin
            runtimePath = std::filesystem::current_path() / "bin" / "ChainedRuntime.exe";
        }

        std::string command = "\"" + runtimePath.string() + "\" \"" + projectFile.string() + "\"";
        CH_CORE_INFO("Launching Standalone: %s", command.c_str());

        if (!ProcessUtils::LaunchProcess(command))
        {
            CH_CORE_ERROR("Failed to launch standalone runtime!");
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

    auto view = m_ActiveScene->GetRegistry().view<PlayerComponent, TransformComponent>();
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
