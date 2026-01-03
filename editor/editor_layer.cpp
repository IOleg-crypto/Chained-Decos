#include "editor_layer.h"
#include "engine/application.h"
#include "engine/components.h"
#include "engine/input.h"
#include "engine/physics.h"
#include "engine/project_serializer.h"
#include "engine/renderer.h"
#include "engine/scene_serializer.h"
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
    rlImGuiSetup(true);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    NFD_Init();

    m_EditorCamera.position = {10.0f, 10.0f, 10.0f};
    m_EditorCamera.target = {0.0f, 0.0f, 0.0f};
    m_EditorCamera.up = {0.0f, 1.0f, 0.0f};
    m_EditorCamera.fovy = 90.0f;
    m_EditorCamera.projection = CAMERA_PERSPECTIVE;

    m_ActiveScene = CreateRef<Scene>();
    m_ActiveScene->CreateEntity("Default Entity");
    m_SceneHierarchyPanel.SetContext(m_ActiveScene);
}

void EditorLayer::OnDetach()
{
    NFD_Quit();
    rlImGuiShutdown();
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        UpdateCamera(&m_EditorCamera, CAMERA_FREE);
    }

    if (m_ActiveScene)
    {
        Physics::Update(m_ActiveScene.get(), deltaTime);
    }
}

void EditorLayer::OnRender()
{
    if (Project::GetActive())
    {
        Renderer::BeginScene(m_EditorCamera);
        Renderer::DrawGrid(20, 1.0f);

        if (m_ActiveScene)
            Renderer::DrawScene(m_ActiveScene.get());

        Renderer::EndScene();
    }
    else
    {
        ClearBackground(DARKGRAY);
    }
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

    // DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

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
        ImGui::EndMenuBar();
    }

    if (Project::GetActive())
    {
        m_SceneHierarchyPanel.OnImGuiRender();
        m_InspectorPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity());
    }

    UI_DrawProjectSelector();

    ImGui::End(); // End DockSpace window

    rlImGuiEnd();
}

void EditorLayer::UI_DrawProjectSelector()
{
    if (Project::GetActive())
        return;

    ImGui::OpenPopup("Project Selector");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200));

    if (ImGui::BeginPopupModal("Project Selector", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::Text("Chained Decos Editor");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Open Project", ImVec2(380, 40)))
            OpenProject();
        if (ImGui::Button("New Project", ImVec2(380, 40)))
            NewProject();
        if (ImGui::Button("Exit", ImVec2(380, 40)))
            Application::Shutdown();

        ImGui::EndPopup();
    }
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
} // namespace CH
