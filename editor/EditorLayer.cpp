#include "EditorLayer.h"
#include "core/Base.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/input/Input.h"
#include "editor/utils/EditorStyles.h"

#include "core/interfaces/ILevelManager.h"
#include "editor/logic/SceneCloner.h"
#include "nfd.h"
#include "scene/resources/map/core/SceneLoader.h"
#include <imgui.h>
#include <raymath.h>
#include <rlImGui.h>

namespace CHEngine
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

void EditorLayer::OnAttach()
{
    CD_INFO("EditorLayer attached");

    m_CameraController = std::make_shared<CameraController>();
    m_CameraController->SetCameraMode(CAMERA_FREE); // Better for editor flying
    m_EditorScene = std::make_shared<GameScene>();
    m_ActiveScene = m_EditorScene;

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(m_ActiveScene);
    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_ViewportPanel = std::make_unique<ViewportPanel>();
    m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_ToolbarPanel = std::make_unique<ToolbarPanel>();
    m_ConsolePanel = std::make_unique<ConsolePanel>();

    // Default Scene Content
    m_ActiveScene = m_EditorScene;
    auto &objects = m_ActiveScene->GetMapObjectsMutable();

    // ground
    MapObjectData ground;
    ground.name = "Ground";
    ground.type = MapObjectType::PLANE;
    ground.size = {100, 100};
    ground.color = DARKGRAY;
    objects.push_back(ground);

    // default cube
    MapObjectData cube;
    cube.name = "Default Cube";
    cube.type = MapObjectType::CUBE;
    cube.position = {0, 0.5f, 0};
    cube.color = WHITE;
    objects.push_back(cube);

    m_HierarchyPanel->SetContext(m_ActiveScene);

    // Apply theme
    EditorStyles::ApplyDarkTheme();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_SceneState == SceneState::Play)
    {
        // Update Game Logic via Engine modules
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager)
        {
            levelManager->Update(deltaTime);
        }
    }

    // Update logic
    if (m_ViewportPanel)
    {
        bool viewportActive = m_ViewportPanel->IsFocused() || m_ViewportPanel->IsHovered();
        m_CameraController->SetInputCaptureBypass(viewportActive);

        if (viewportActive)
            m_CameraController->Update();
    }
}

void EditorLayer::OnRender()
{
    // This is called by Engine::Render
    // We render the viewport here if needed, or in OnImGuiRender
}

void EditorLayer::OnImGuiRender()
{
    UI_DrawDockspace();
    m_ToolbarPanel->OnImGuiRender(this);

    // Panels
    m_HierarchyPanel->OnImGuiRender(this);
    m_InspectorPanel->OnImGuiRender(GetSelectedObject());
    if (m_ViewportPanel)
        m_ViewportPanel->OnImGuiRender(m_ActiveScene, m_CameraController, this);
    m_AssetBrowserPanel->OnImGuiRender();
    m_ConsolePanel->OnImGuiRender();
}

void EditorLayer::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(CD_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        CD_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
{
    // Shortcuts (Hazel style)
    if (e.GetRepeatCount() > 0)
        return false;

    bool control = Input::IsKeyDown(KEY_LEFT_CONTROL) || Input::IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyDown(KEY_LEFT_SHIFT) || Input::IsKeyDown(KEY_RIGHT_SHIFT);
    if (control)
    {
        switch (e.GetKeyCode())
        {
        case KEY_N:
            NewScene();
            break;
        case KEY_O:
            OpenScene();
            break;
        case KEY_S:
            if (shift)
                SaveSceneAs();
            else
                SaveScene();
            break;
        }
    }
    else
    {
        switch (e.GetKeyCode())
        {
        case KEY_Q:
            SetActiveTool(Tool::SELECT);
            break;
        case KEY_W:
            SetActiveTool(Tool::MOVE);
            break;
        case KEY_E:
            SetActiveTool(Tool::ROTATE);
            break;
        case KEY_R:
            SetActiveTool(Tool::SCALE);
            break;
        case KEY_DELETE:
            if (m_SelectedObjectIndex >= 0 && m_ActiveScene)
            {
                auto &objects = m_ActiveScene->GetMapObjectsMutable();
                if (m_SelectedObjectIndex < (int)objects.size())
                {
                    objects.erase(objects.begin() + m_SelectedObjectIndex);
                    m_SelectedObjectIndex = -1;
                }
            }
            break;
        }
    }

    return false;
}

bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    // Object picking is now handled by ViewportPanel::OnImGuiRender
    // to ensure correct coordinate mapping within the render texture's 3D context.
    return false;
}

void EditorLayer::UI_DrawDockspace()
{
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + 32.0f));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - 32.0f));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
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
            if (ImGui::MenuItem("New", "Ctrl+N"))
                NewScene();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                OpenScene();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveScene();
            if (ImGui::MenuItem("Exit"))
                Engine::Instance().RequestExit();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;
    CD_INFO("Scene Play started");

    // 1. Save current state to temp
    if (m_ActiveScene)
    {
        std::string tempPath = SceneCloner::GetTempPath();
        if (SceneCloner::SaveTemp(*m_ActiveScene, tempPath))
        {
            // 2. Load into Engine/LevelManager for runtime simulation
            auto levelManager = Engine::Instance().GetService<ILevelManager>();
            if (levelManager)
            {
                levelManager->LoadScene(tempPath);
                CD_INFO("Runtime scene loaded from temp");
            }
        }
    }
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;
    CD_INFO("Scene Play stopped");

    // Restore original scene state from temp backup
    std::string tempPath = SceneCloner::GetTempPath();
    SceneLoader loader;
    auto original = loader.LoadScene(tempPath);
    if (!original.GetMapObjects().empty())
    {
        *m_ActiveScene = std::move(original);
        CD_INFO("Editor scene restored from backup");
    }
}

MapObjectData *EditorLayer::GetSelectedObject()
{
    if (m_SelectedObjectIndex < 0 || !m_ActiveScene)
        return nullptr;

    auto &objects = m_ActiveScene->GetMapObjectsMutable();
    if (m_SelectedObjectIndex >= (int)objects.size())
        return nullptr;

    return &objects[m_SelectedObjectIndex];
}

void EditorLayer::UI_DrawToolbar()
{
    // MOVED TO ToolbarPanel
}

void EditorLayer::NewScene()
{
    m_EditorScene = std::make_shared<GameScene>();
    m_ActiveScene = m_EditorScene;
    m_ScenePath = "";
    m_HierarchyPanel->SetContext(m_ActiveScene);
}

void EditorLayer::OpenScene()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "json"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

    if (result == NFD_OKAY)
    {
        SceneLoader loader;
        m_EditorScene = std::make_shared<GameScene>(loader.LoadScene(outPath));
        m_ActiveScene = m_EditorScene;
        m_ScenePath = outPath;
        m_HierarchyPanel->SetContext(m_ActiveScene);
        NFD_FreePath(outPath);
    }
}

void EditorLayer::SaveScene()
{
    if (m_ScenePath.empty())
    {
        SaveSceneAs();
    }
    else
    {
        SceneLoader loader;
        loader.SaveScene(*m_EditorScene, m_ScenePath);
    }
}

void EditorLayer::SaveSceneAs()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "json"}};
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json");

    if (result == NFD_OKAY)
    {
        m_ScenePath = outPath;
        SceneLoader loader;
        loader.SaveScene(*m_EditorScene, m_ScenePath);
        NFD_FreePath(outPath);
    }
}
} // namespace CHEngine
