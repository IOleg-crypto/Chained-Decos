#include "EditorLayer.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "core/Base.h"
#include "core/Log.h"
#include "core/application/Application.h"
#include "core/input/Input.h"
#include "core/renderer/Renderer.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/panels/ContentBrowserPanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/MenuBarPanel.h"
#include "editor/panels/ProjectBrowserPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "editor/utils/EditorStyles.h"
#include "editor/utils/ProcessUtils.h"
#include "events/MouseEvent.h"
#include "scene/MapManager.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/core/IDComponent.h"
#include "scene/ecs/components/core/TagComponent.h"
#include <cstdlib>

#include <fstream>

#include "core/interfaces/IPlayer.h"
#include "core/physics/Physics.h"
#include "editor/logic/SceneCloner.h"
#include "editor/logic/undo/AddObjectCommand.h"
#include "editor/logic/undo/DeleteObjectCommand.h"
#include "editor/logic/undo/ModifyObjectCommand.h"
#include "editor/logic/undo/TransformCommand.h"
#include "imgui_internal.h"
#include "nfd.h"
#include "raylib.h"
#include "runtime/RuntimeLayer.h"
#include "scene/core/SceneSerializer.h"
#include "scene/main/LevelManager.h"
#include "scene/resources/map/SceneSerializer.h"
#include "scene/resources/model/Model.h"
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

    // Initialize new Scene system
    m_Scene = std::make_shared<Scene>("GameScene");
    m_UIScene = std::make_shared<Scene>("UIScene");
    CD_INFO("[EditorLayer] Created editor scenes: %s, %s", m_Scene->GetName().c_str(),
            m_UIScene->GetName().c_str());

    if (LevelManager::IsInitialized())
    {
        LevelManager::SetActiveScene(m_Scene);
        // LevelManager::SetUIScene(m_UIScene); // If LevelManager supports it
    }

    // Create default entities using raw entt::entity (temporary workaround for circular dependency)
    auto &registry = m_Scene->GetRegistry();

    // Legacy GameScene initialization
    m_ProjectManager.SetSceneChangedCallback(
        [this](const std::shared_ptr<CHEngine::GameScene> &scene)
        {
            if (MapManager::IsInitialized())
                MapManager::SetCurrentScene(scene);

            if (m_PanelManager)
            {
                auto hierarchy = m_PanelManager->GetPanel<HierarchyPanel>("Hierarchy");
                if (hierarchy)
                    hierarchy->SetContext(scene);
            }
        });

    // Initialize Core Logic
    m_SceneActions = std::make_unique<EditorSceneActions>(nullptr, &m_SimulationManager, m_Scene,
                                                          &m_RuntimeLayer);
    m_EntityFactory =
        std::make_unique<EditorEntityFactory>(m_Scene, m_CommandHistory, m_SelectionManager);

    // Initialize Panel Manager
    m_PanelManager = std::make_unique<PanelManager>();
    InitPanels();

    m_ProjectActions = std::make_unique<EditorProjectActions>(&m_ProjectManager, nullptr, nullptr,
                                                              &m_ShowProjectBrowser);

    m_Input = std::make_unique<EditorInput>(
        m_SceneActions.get(), m_EntityFactory.get(), &m_CommandHistory, &m_SelectionManager,
        &m_SimulationManager, EditorInput::Callbacks{[this](Tool t) { SetActiveTool(t); }});

    EditorStyles::ApplyDarkTheme();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Update Game Logic via Engine modules
        if (LevelManager::IsInitialized())
        {
            LevelManager::Update(deltaTime);
        }

        // Note: Player update is now handled by ECS systems in RuntimeLayer
    }

    // Update logic
    if (m_PanelManager)
    {
        auto vp = m_PanelManager->GetPanel<ViewportPanel>("Viewport");
        if (vp)
        {
            bool viewportActive = vp->IsFocused() || vp->IsHovered();
            m_EditorCamera.SetViewportSize(vp->GetSize().x, vp->GetSize().y);

            if (viewportActive && m_SimulationManager.GetSceneState() == SceneState::Edit)
                m_EditorCamera.OnUpdate(deltaTime);
        }
    }

    // Simulation input handling
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

        // Sync Scene Entities to Physics System
        if (CollisionManager::IsInitialized())
        {
            auto &registry = m_Scene->GetRegistry();
            auto view = registry.view<TransformComponent, CollisionComponent>();

            view.each(
                [&](auto entity, auto &transform, auto &collision)
                {
                    if (collision.collider)
                    {
                        // Update collider position from transform
                        collision.collider->Update(transform.position,
                                                   collision.collider->GetSize());
                    }
                });
        }
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
    if (m_PanelManager)
        m_PanelManager->OnImGuiRender();
}

void EditorLayer::InitPanels()
{
    m_PanelManager->AddPanel<HierarchyPanel>("Hierarchy", m_Scene, &m_SelectionManager,
                                             m_EntityFactory.get(), &m_CommandHistory);
    m_PanelManager->AddPanel<InspectorPanel>("Inspector", &m_SelectionManager, &m_CommandHistory);
    m_PanelManager->AddPanel<ViewportPanel>("Viewport", m_SceneActions.get(), &m_SelectionManager,
                                            &m_SimulationManager, &m_EditorCamera,
                                            m_EntityFactory.get(), &m_CommandHistory);
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

MapObjectData *EditorLayer::GetSelectedObject()
{
    return m_SelectionManager.GetSelectedObject(MapManager::GetCurrentScene());
}

UIElementData *EditorLayer::GetSelectedUIElement()
{
    return m_SelectionManager.GetSelectedUIElement(MapManager::GetCurrentScene());
}

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
    auto activeScene = MapManager::GetCurrentScene();
    auto skybox = std::make_shared<Skybox>();
    skybox->Init();
    skybox->LoadMaterialTexture(path);
    activeScene->SetSkyBox(skybox);
    activeScene->GetMapMetaDataMutable().skyboxTexture = path;
    CD_INFO("Skybox applied: %s", path.c_str());
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
    std::string runtimePath = std::string(PROJECT_ROOT_DIR) + "/build/bin/Runtime.exe";

    // Build command line with --map and --skip-menu arguments
    std::string commandLine = "\"" + runtimePath + "\" --map \"" + tempScenePath + "\" --skip-menu";

    CD_INFO("[EditorLayer] Launching: %s", commandLine.c_str());

    // Launch runtime process using ProcessUtils (non-blocking)
    if (ProcessUtils::LaunchProcess(commandLine, PROJECT_ROOT_DIR))
    {
        CD_INFO("[EditorLayer] Runtime launched successfully");
    }
    else
    {
        CD_ERROR("[EditorLayer] Failed to launch runtime");
    }
}

} // namespace CHEngine
