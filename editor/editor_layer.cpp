#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/physics/physics.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "raylib.h"
#include "ui/project_selector.h"
#include "ui/toolbar.h"
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>
#include <nfd.h>
#include <rlImGui.h>

namespace CH
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

void EditorLayer::OnAttach()
{
    rlImGuiBeginInitImGui();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Load legacy fonts
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Regular.ttf", 18.0f);

    static const ImWchar icons_ranges[] = {0xe005, 0xf8ff, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/fa-solid-900.ttf", 18.0f,
                                 &icons_config, icons_ranges);

    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 20.0f);
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/Lato-Bold.ttf", 26.0f);

    rlImGuiEndInitImGui();
    // ImGui::StyleColorsDark();
    SetDarkThemeColors();

    NFD_Init();

    m_EditorCamera.SetPosition({10.0f, 10.0f, 10.0f});
    m_EditorCamera.SetTarget({0.0f, 0.0f, 0.0f});
    m_EditorCamera.SetFOV(90.0f);

    m_ActiveScene = CreateRef<Scene>();
    m_ActiveScene->CreateEntity("Default Entity");
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);

    m_ProjectBrowserPanel.SetOnProjectOpen([this](const std::string &path) { OpenProject(path); });
    m_ProjectBrowserPanel.SetOnProjectCreate(
        [this](const std::string &name, const std::string &path)
        {
            NewProject(); // This is a bit simplified, but fine for now
        });

    m_AppIcon = LoadTexture(PROJECT_ROOT_DIR "/resources/icons/ChainedDecosMapEditor.png");
}

void EditorLayer::OnDetach()
{
    UnloadTexture(m_AppIcon);
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
            // Undo/Redo
            bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            if (control)
            {
                if (IsKeyPressed(KEY_Z))
                    m_CommandHistory.Undo();
                if (IsKeyPressed(KEY_Y))
                    m_CommandHistory.Redo();
            }

            // Gizmo tool selection (Q/W/E/R)
            if (Input::IsKeyPressed(KEY_Q))
                m_CurrentTool = GizmoType::SELECT;
            if (Input::IsKeyPressed(KEY_W))
                m_CurrentTool = GizmoType::TRANSLATE;
            if (Input::IsKeyPressed(KEY_E))
                m_CurrentTool = GizmoType::ROTATE;
            if (Input::IsKeyPressed(KEY_R))
                m_CurrentTool = GizmoType::SCALE;
        }

        if (m_ActiveScene)
        {
            Physics::Update(m_ActiveScene.get(), deltaTime);

            // Entity picking
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse)
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
            Physics::Update(m_ActiveScene.get(), deltaTime);
            m_ActiveScene->OnUpdateRuntime(deltaTime);
        }
    }
}

void EditorLayer::OnRender()
{
    // Render logic is now handled by ViewportPanel through its own RenderTexture.
    // We only need to clear the main window color here if dockspace is transparent.
    ClearBackground(BLACK);
}

void EditorLayer::OnImGuiRender()
{
    rlImGuiBegin();

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

    // MenuBar must be drawn first if we want it at the very top of the window
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
            // Panel visibility toggles (always shown for now, could add flags later)
            ImGui::MenuItem("Viewport", nullptr, true);
            ImGui::MenuItem("Scene Hierarchy", nullptr, true);
            ImGui::MenuItem("Inspector", nullptr, true);
            ImGui::MenuItem("Environment", nullptr, &m_EnvironmentPanel.IsOpen());
            ImGui::MenuItem("Content Browser", nullptr, true);
            ImGui::MenuItem("Console", nullptr, true);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scripting"))
        {
            if (ImGui::MenuItem("Reload Assemblies", "Ctrl+R"))
            {
                // TODO: Call CSharp scripting reload logic
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Toolbar
    if (Project::GetActive())
    {
        UI::DrawToolbar(
            m_SceneState == SceneState::Play, [this]() { OnScenePlay(); },
            [this]() { OnSceneStop(); });
    }

    // DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (Project::GetActive())
    {
        m_ViewportPanel.OnImGuiRender(m_ActiveScene.get(), GetActiveCamera(),
                                      m_SceneHierarchyPanel.GetSelectedEntity(), m_CurrentTool,
                                      m_Gizmo);
        m_SceneHierarchyPanel.OnImGuiRender();
        m_ContentBrowserPanel.OnImGuiRender();
        m_ConsolePanel.OnImGuiRender();
        m_EnvironmentPanel.OnImGuiRender(m_ActiveScene.get());
        m_InspectorPanel.OnImGuiRender(m_ActiveScene.get(),
                                       m_SceneHierarchyPanel.GetSelectedEntity());
    }
    else
    {
        m_ProjectBrowserPanel.OnImGuiRender();
    }

    ImGui::End(); // End DockSpace window

    rlImGuiEnd();
}

void EditorLayer::NewProject()
{
    nfdchar_t *savePath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, NULL, "Untitled.chproject");
    if (result == NFD_OKAY)
    {
        Ref<Project> newProject = CreateRef<Project>();
        newProject->SetName(std::filesystem::path(savePath).stem().string());
        ProjectSerializer serializer(newProject);
        serializer.Serialize(savePath);
        Project::SetActive(newProject);
        CH_CORE_INFO("Project Created: %s", savePath);

        // Auto-create asset folder
        std::filesystem::path projectDir = std::filesystem::path(savePath).parent_path();
        std::filesystem::create_directories(projectDir / "assets/scenes");

        NFD_FreePath(savePath);
        NewScene();
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

void EditorLayer::OpenProject(const std::filesystem::path &path)
{
    Ref<Project> project = CreateRef<Project>();
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        Project::SetActive(project);
        CH_CORE_INFO("Project Opened: %s", path.string().c_str());

        auto &config = project->GetConfig();
        if (!config.ActiveScenePath.empty())
        {
            OpenScene(project->GetConfig().ProjectDirectory / config.ActiveScenePath);
        }
        else
        {
            NewScene();
        }
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
        serializer.Serialize(projectFile);
        CH_CORE_INFO("Project Saved: %s", projectFile.string().c_str());
    }
}

void EditorLayer::NewScene()
{
    m_ActiveScene = CreateRef<Scene>();
    m_ActiveScene->CreateEntity("Default Entity");
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);

    auto project = Project::GetActive();
    if (project)
    {
        // Don't set ActiveScenePath yet, wait for first save
    }
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
            // Update active scene path relative to project
            std::error_code ec;
            auto relPath =
                std::filesystem::relative(path, project->GetConfig().ProjectDirectory, ec);
            if (!ec)
            {
                std::error_code ec;
                auto relPath =
                    std::filesystem::relative(path, project->GetConfig().ProjectDirectory, ec);
                if (!ec)
                {
                    project->SetActiveScenePath(relPath);
                }
            }
        }
    }
}

void EditorLayer::SaveScene()
{
    auto project = Project::GetActive();
    if (project && !project->GetConfig().ActiveScenePath.empty())
    {
        SceneSerializer serializer(m_ActiveScene.get());
        serializer.Serialize(
            (project->GetConfig().ProjectDirectory / project->GetConfig().ActiveScenePath)
                .string());
    }
    else
    {
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
        serializer.Serialize(savePath);

        auto project = Project::GetActive();
        if (project)
        {
            std::error_code ec;
            auto relPath =
                std::filesystem::relative(savePath, project->GetConfig().ProjectDirectory, ec);
            if (!ec)
            {
                project->SetActiveScenePath(relPath);
            }
        }
        NFD_FreePath(savePath);
    }
}

void EditorLayer::OnEvent(Event &e)
{
}
void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;

    // Hide spawn zone during gameplay
    auto spawnView = m_ActiveScene->GetRegistry().view<SpawnComponent>();
    for (auto entity : spawnView)
    {
        auto &spawn = spawnView.get<SpawnComponent>(entity);
        spawn.RenderSpawnZoneInScene = false;
    }
    if (Input::IsKeyPressed(KEY_F1))
    {
        auto playerView = m_ActiveScene->GetRegistry().view<PlayerComponent>();
        for (auto entity : playerView)
        {
            auto &player = playerView.get<PlayerComponent>(entity);
            DrawCubeWires(player.coordinates, 5.0f, 5.0f, 5.0f, RED);
        }
    }

    CH_CORE_INFO("Scene Started.");
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;

    // Restore spawn zone visibility in editor
    auto spawnView = m_ActiveScene->GetRegistry().view<SpawnComponent>();
    for (auto entity : spawnView)
    {
        auto &spawn = spawnView.get<SpawnComponent>(entity);
        spawn.RenderSpawnZoneInScene = true;
    }

    CH_CORE_INFO("Scene Stopped.");
}

void EditorLayer::SetDarkThemeColors()
{
    auto &colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
}
CommandHistory &EditorLayer::GetCommandHistory()
{
    // In V2 Application manages layers, but for migration we need access.
    // For now, we get the last layer from the stack, assuming it's the EditorLayer.
    auto &layers = Application::Get().GetLayerStack().GetLayers();
    return ((EditorLayer *)layers.back())->m_CommandHistory;
}

Camera3D EditorLayer::GetActiveCamera()
{
    if (m_SceneState == SceneState::Edit)
        return m_EditorCamera.GetRaylibCamera();

    // Runtime Player Camera
    auto view = m_ActiveScene->GetRegistry().view<PlayerComponent, TransformComponent>();
    if (view.begin() != view.end())
    {
        auto entity = *view.begin();
        auto &transform = view.get<TransformComponent>(entity);
        auto &player = view.get<PlayerComponent>(entity);

        // Mouse control (internal to this state for follow camera)
        Vector2 mouseDelta = GetMouseDelta();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            player.CameraYaw -= mouseDelta.x * player.LookSensitivity;
            player.CameraPitch -= mouseDelta.y * player.LookSensitivity;

            if (player.CameraPitch > 89.0f)
                player.CameraPitch = 89.0f;
            if (player.CameraPitch < -10.0f)
                player.CameraPitch = -10.0f;
        }

        player.CameraDistance -= GetMouseWheelMove() * 2.0f;
        if (player.CameraDistance < 2.0f)
            player.CameraDistance = 2.0f;
        if (player.CameraDistance > 40.0f)
            player.CameraDistance = 40.0f;

        Vector3 target = transform.Translation;
        target.y += 1.0f; // Look at head

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
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        player.coordinates = camera.position;
        return camera;
    }

    return m_EditorCamera.GetRaylibCamera();
}
} // namespace CH
