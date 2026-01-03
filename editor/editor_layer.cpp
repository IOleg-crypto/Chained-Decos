#include "editor_layer.h"
#include "components/physics/collision/core/collision_manager.h"
#include "core/application/application.h"
#include "core/input/input.h"
#include "core/log.h"
#include "core/renderer/renderer.h"
#include "core/utils/base.h"
#include "editor/panels/console_panel.h"
#include "editor/panels/content_browser_panel.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/inspector_panel.h"
#include "editor/panels/menu_bar_panel.h"
#include "editor/panels/project_browser_panel.h"
#include "editor/panels/toolbar_panel.h"
#include "editor/panels/viewport_panel.h"
#include "editor/utils/editor_styles.h"
#include "editor/utils/process_utils.h"
#include "events/mouse_event.h"
#include "scene/core/scene_manager.h"
#include "scene/ecs/components/camera_component.h"
#include "scene/ecs/components/core/id_component.h"
#include "scene/ecs/components/core/tag_component.h"
#include "scene/ecs/components/physics_data.h"
#include "scene/ecs/components/skybox_component.h"
#include "scene/ecs/components/transform_component.h"

#include <fstream>

#include "components/physics/collision/core/physics.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/editor_scene_manager.h"
#include "editor/logic/scene_cloner.h"
#include "imgui_internal.h"
#include "nfd.h"
#include "raylib.h"
#include "runtime/runtime_layer.h"
#include "scene/core/scene_serializer.h"
#include "scene/resources/model/model.h"
#include <filesystem>

namespace CHEngine
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

// =========================================================================
// Layer Lifecycle
// =========================================================================

void EditorLayer::OnAttach()
{
    CD_INFO("EditorLayer attached");

    // Register Raylib log callback to redirect logs to ConsolePanel
    SetTraceLogCallback(
        [](int logLevel, const char *text, va_list args)
        {
            char buffer[4096];
            vsnprintf(buffer, sizeof(buffer), text, args);

            LogMessage::Level level = LogMessage::Level::Info;
            switch (logLevel)
            {
            case LOG_WARNING:
                level = LogMessage::Level::Warn;
                break;
            case LOG_ERROR:
            case LOG_FATAL:
                level = LogMessage::Level::Error;
                break;
            default:
                level = LogMessage::Level::Info;
                break;
            }

            ConsolePanel::AddLog(buffer, level);
        });

    // Initialize scenes as null initially (will be loaded by project/manager)
    m_Scene = nullptr;
    m_UIScene = nullptr;

    // Legacy ProjectManager setup
    m_ProjectManager.SetSceneChangedCallback(
        [this](const std::shared_ptr<CHEngine::Scene> &scene)
        {
            m_Scene = scene; // CRITICAL: Update EditorLayer's own scene pointer

            SceneManager::LoadScene(scene);

            if (m_SceneActions)
                m_SceneActions->SetScene(scene);
            if (m_EntityFactory)
                m_EntityFactory->SetScene(scene);

            if (m_PanelManager)
            {
                auto hierarchy = m_PanelManager->GetPanelTyped<HierarchyPanel>("Hierarchy");
                if (hierarchy)
                    hierarchy->SetContext(scene);
            }
        });

    // Initialize Core Logic
    m_SceneManager = std::make_unique<EditorSceneManager>(&m_SelectionManager);
    m_SceneActions = std::make_unique<EditorSceneActions>(
        m_SceneManager.get(), &m_SimulationManager, m_Scene, &m_RuntimeLayer);
    m_EntityFactory = std::make_unique<EditorEntityFactory>(
        m_Scene, m_CommandHistory, m_SelectionManager, m_SceneManager.get());

    // Initialize Panel Manager
    m_PanelManager = std::make_unique<PanelManager>();
    InitPanels();

    m_ProjectActions = std::make_unique<EditorProjectActions>(&m_ProjectManager, nullptr, nullptr,
                                                              &m_ShowProjectBrowser);

    m_Input = std::make_unique<EditorInput>(
        m_SceneActions.get(), m_EntityFactory.get(), &m_CommandHistory, &m_SelectionManager,
        &m_SimulationManager, m_SceneManager.get(),
        EditorInput::Callbacks{[this](Tool t) { SetActiveTool(t); }});

    EditorStyles::ApplyDarkTheme();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_Scene && m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Update ECS runtime
        m_Scene->OnUpdateRuntime(deltaTime);
    }

    // Update logic
    if (m_PanelManager)
    {
        auto vp = m_PanelManager->GetPanelTyped<ViewportPanel>("Viewport");
        if (vp)
        {
            bool viewportActive = vp->IsFocused() || vp->IsHovered();
            m_EditorCamera.SetViewportSize(vp->GetSize().x, vp->GetSize().y);

            if (viewportActive && m_SimulationManager.GetSceneState() == SceneState::Edit)
                m_EditorCamera.OnUpdate(deltaTime);
        }
    }

    // Simulation input.handling
    if (m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Toggle cursor with ESCAPE
        if (Input::IsKeyPressed(KEY_ESCAPE))
        {
            m_CursorLocked = !m_CursorLocked;
            if (m_CursorLocked)
                DisableCursor();
            else
                EnableCursor();
        }

        // Emergency stop with BACKSPACE
        if (Input::IsKeyPressed(KEY_BACKSPACE))
        {
            m_SceneActions->OnSceneStop();
            return;
        }
    }

    // Update New Scene Architecture
    if (m_Scene)
    {
        m_Scene->OnUpdateEditor(deltaTime);

        // Physics update (placeholder for PhysicsSystem)
        // TODO: Move to a dedicated system that runs in OnUpdateEditor/Runtime
    }
}

// =========================================================================
// Rendering & UI
// =========================================================================

void EditorLayer::OnRender()
{
}

void EditorLayer::OnImGuiRender()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
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

    // Submit the DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    // OPTIONAL: Only show the dockspace/panels if a project is loaded
    bool hasProject = m_ProjectManager.GetActiveProject() != nullptr;

    if (hasProject)
    {
        if (m_PanelManager)
        {
            // MenuBar rendering
            auto menuBar = m_PanelManager->GetPanelTyped<MenuBarPanel>("MenuBar");
            if (menuBar && menuBar->IsVisible())
                menuBar->OnImGuiRender();

            // Render all other panels
            m_PanelManager->OnImGuiRender();
        }
    }
    else
    {
        // Project Manager View - only show Project Browser Panel
        if (m_PanelManager)
        {
            auto projectBrowser = m_PanelManager->GetPanel("Project Browser");
            if (projectBrowser && projectBrowser->IsVisible())
            {
                projectBrowser->OnImGuiRender();
            }
        }
    }

    ImGui::End();
}

void EditorLayer::InitPanels()
{
    m_PanelManager->AddPanel<HierarchyPanel>("Hierarchy", m_Scene, &m_SelectionManager,
                                             m_EntityFactory.get(), &m_CommandHistory);
    m_PanelManager->AddPanel<InspectorPanel>("Inspector", &m_SelectionManager, &m_CommandHistory,
                                             m_SceneManager.get(), m_SceneActions.get());
    m_PanelManager->AddPanel<ViewportPanel>(
        "Viewport", m_SceneActions.get(), m_ProjectActions.get(), &m_SelectionManager,
        &m_SimulationManager, &m_EditorCamera, m_EntityFactory.get(), &m_CommandHistory,
        &m_ActiveTool);
    m_PanelManager->AddPanel<ToolbarPanel>("Toolbar", m_SceneActions.get(), &m_SimulationManager,
                                           &m_ActiveTool);
    m_PanelManager->AddPanel<ContentBrowserPanel>("Content Browser", m_SceneActions.get());
    m_PanelManager->AddPanel<ConsolePanel>("Console");
    m_PanelManager->AddPanel<ProjectBrowserPanel>("Project Browser", m_ProjectActions.get());
    m_PanelManager->AddPanel<MenuBarPanel>("MenuBar", m_SceneActions.get(), m_ProjectActions.get(),
                                           &m_CommandHistory, m_PanelManager.get(),
                                           &m_ShowProjectSettings);
}

// =========================================================================
// Events & Input
// =========================================================================

void EditorLayer::OnEvent(Event &event)
{
    if (m_SimulationManager.GetSceneState() == SceneState::Edit)
        m_EditorCamera.OnEvent(event);

    if (m_Input && m_Input->OnEvent(event))
        return;
}

// =========================================================================
// Scene Commands
// =========================================================================

// =========================================================================
// Environment & Skybox
// =========================================================================

void EditorLayer::LoadSkybox(const std::string &path)
{
    if (path.empty())
    {
        nfdfilteritem_t filterItem[1] = {{"Skybox Texture (HDR, PNG, JPG)", "hdr,png,jpg,jpeg"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            std::string selectedPath = outPath;
            NFD_FreePath(outPath);
            ApplySkybox(selectedPath);
        }
    }
    else
    {
        ApplySkybox(path);
    }
}

void EditorLayer::ApplySkybox(const std::string &path)
{
    // ECS implementation: Find global settings entity or create one
    auto view = m_Scene->GetRegistry().view<SkyboxComponent>();
    entt::entity settingsEntity =
        view.empty() ? m_Scene->CreateEntity("GlobalSettings") : view.front();

    auto &skybox = m_Scene->GetRegistry().emplace_or_replace<SkyboxComponent>(settingsEntity);
    skybox.TexturePath = path;

    CD_INFO("Skybox applied to ECS: %s", path.c_str());
}

// =========================================================================
// Getters & Setters
// =========================================================================

SceneState EditorLayer::GetSceneState() const
{
    return m_SimulationManager.GetSceneState();
}

SelectionManager &EditorLayer::GetSelectionManager()
{
    return m_SelectionManager;
}

Tool EditorLayer::GetActiveTool() const
{
    return m_ActiveTool;
}

void EditorLayer::SetActiveTool(Tool tool)
{
    m_ActiveTool = tool;
}

std::shared_ptr<Scene> EditorLayer::GetActiveScene()
{
    return m_Scene;
}

void EditorLayer::PlayInRuntime()
{
    if (!m_Scene)
    {
        CD_WARN("[EditorLayer] No active scene to play in runtime");
        return;
    }

    // Use project's scene directory if available, otherwise fallback to project root
    std::string tempScenePath;
    auto activeProject = m_ProjectManager.GetActiveProject();
    if (activeProject)
    {
        std::filesystem::path sceneDir = activeProject->GetSceneDirectory();
        if (!std::filesystem::exists(sceneDir))
        {
            std::filesystem::create_directories(sceneDir);
        }
        tempScenePath = (sceneDir / "RuntimeScene.chscene").string();
    }
    else
    {
        tempScenePath = (std::filesystem::path(PROJECT_ROOT_DIR) / "RuntimeScene.chscene").string();
    }

    CD_INFO("[EditorLayer] Saving runtime scene to: %s", tempScenePath.c_str());

    // Use ECSSceneSerializer to save the ECS scene
    CHEngine::ECSSceneSerializer serializer(m_Scene);
    serializer.Serialize(tempScenePath);

    CD_INFO("[EditorLayer] Scene saved successfully, launching runtime...");

    // Build runtime executable path
    std::filesystem::path engineRuntime =
        std::filesystem::path(PROJECT_ROOT_DIR) / "build/bin/Runtime.exe";
    std::filesystem::path runtimePath;

    if (activeProject)
    {
        runtimePath = activeProject->GetProjectDirectory() / "Runtime.exe";
        if (!std::filesystem::exists(runtimePath) && std::filesystem::exists(engineRuntime))
        {
            try
            {
                std::filesystem::copy_file(engineRuntime, runtimePath,
                                           std::filesystem::copy_options::overwrite_existing);
                CD_INFO("[EditorLayer] Copied Runtime.exe to project directory: %s",
                        runtimePath.string().c_str());
            }
            catch (const std::exception &e)
            {
                CD_ERROR("[EditorLayer] Failed to copy runtime to project: %s", e.what());
                runtimePath = engineRuntime; // Fallback
            }
        }
        else if (!std::filesystem::exists(runtimePath))
        {
            runtimePath = engineRuntime;
        }
    }
    else
    {
        runtimePath = engineRuntime;
    }

    // Build command line with --map and --skip-menu arguments
    std::string commandLine =
        "\"" + runtimePath.string() + "\" --map \"" + tempScenePath + "\" --skip-menu";

    CD_INFO("[EditorLayer] Launching: %s", commandLine.c_str());

    // Launch runtime process using ProcessUtils (non-blocking)
    std::string workingDir =
        activeProject ? activeProject->GetProjectDirectory().string() : PROJECT_ROOT_DIR;
    if (ProcessUtils::LaunchProcess(commandLine, workingDir))
    {
        CD_INFO("[EditorLayer] Runtime launched successfully from %s", workingDir.c_str());
    }
    else
    {
        CD_ERROR("[EditorLayer] Failed to launch runtime");
    }
}

} // namespace CHEngine
