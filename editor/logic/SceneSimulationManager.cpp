#include "SceneSimulationManager.h"
#include "core/Base.h"
#include "core/Engine.h"
#include "core/application/EngineApplication.h"
#include "core/interfaces/ILevelManager.h"
#include "core/physics/Physics.h"
#include "editor/logic/SceneCloner.h"
#include "editor/utils/ProcessUtils.h"
#include "project/Project.h"
#include "runtime/RuntimeLayer.h"
#include "runtime/logic/RuntimeInitializer.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/map/SceneSerializer.h"
#include <cstdlib>
#include <filesystem>
#include <raylib.h>

namespace CHEngine
{

// =========================================================================
// Getters & Setters
// =========================================================================

SceneState SceneSimulationManager::GetSceneState() const
{
    return m_SceneState;
}

void SceneSimulationManager::SetSceneState(SceneState state)
{
    m_SceneState = state;
}

RuntimeMode SceneSimulationManager::GetRuntimeMode() const
{
    return m_RuntimeMode;
}

void SceneSimulationManager::SetRuntimeMode(RuntimeMode mode)
{
    m_RuntimeMode = mode;
}

// =========================================================================
// Simulation Lifecycle
// =========================================================================

SceneSimulationManager::SceneSimulationManager() = default;

void SceneSimulationManager::OnScenePlay(std::shared_ptr<GameScene> &activeScene,
                                         std::shared_ptr<GameScene> &editorScene,
                                         std::shared_ptr<Scene> &newScene, RuntimeMode runtimeMode,
                                         CHD::RuntimeLayer **runtimeLayer, EngineApplication *app)
{
    m_SceneState = SceneState::Play;
    m_RuntimeMode = runtimeMode;

    CD_INFO("Scene Play started (Mode: %s)",
            m_RuntimeMode == RuntimeMode::Standalone ? "Standalone" : "Embedded");

    // 1. Find Spawn Point
    Vector3 spawnPos = {0, 5, 0};
    for (const auto &obj : activeScene->GetMapObjects())
    {
        if (obj.type == MapObjectType::SPAWN_ZONE)
        {
            spawnPos = obj.position;
            CD_INFO("Found Spawn Zone at (%.2f, %.2f, %.2f)", spawnPos.x, spawnPos.y, spawnPos.z);
            break;
        }
    }

    // 2. Save current state to temp for simulation
    std::string tempPath;
    auto levelManager = Engine::Instance().GetService<ILevelManager>();

    // Try to find the project to save in the project's scenes directory
    // This is a bit tricky from here, but we can look for .chproject files nearby or use
    // levelManager For now, let's look for any project file in the root subdirectories
    std::filesystem::path root(PROJECT_ROOT_DIR);
    std::filesystem::path sceneDir;

    try
    {
        for (auto const &dir_entry : std::filesystem::recursive_directory_iterator(root))
        {
            if (dir_entry.path().extension() == ".chproject")
            {
                auto project = Project::Load(dir_entry.path());
                if (project)
                {
                    sceneDir = project->GetSceneDirectory();
                    break;
                }
            }
        }
    }
    catch (...)
    {
    }

    if (!sceneDir.empty())
    {
        if (!std::filesystem::exists(sceneDir))
        {
            std::filesystem::create_directories(sceneDir);
        }
        tempPath = (sceneDir / "test.chscene").string();
    }
    else
    {
        tempPath = (root / "test.chscene").string();
    }

    CD_INFO("[SceneSimulationManager] Saving runtime scene to: %s", tempPath.c_str());

    SceneSerializer serializer(activeScene);
    if (serializer.SerializeBinary(tempPath))
    {
        if (m_RuntimeMode == RuntimeMode::Standalone)
        {
            std::filesystem::path runtimePath =
                std::filesystem::path(PROJECT_ROOT_DIR) / "build" / "bin" / "Runtime.exe";

            // Build command line with --map and --skip-menu
            std::string commandLine =
                "\"" + runtimePath.string() + "\" --map \"" + tempPath + "\" --skip-menu";

            CD_INFO("[SceneSimulationManager] Launching standalone runtime: %s",
                    commandLine.c_str());

            if (ProcessUtils::LaunchProcess(commandLine, PROJECT_ROOT_DIR))
            {
                CD_INFO("[SceneSimulationManager] Standalone runtime process started successfully");
            }
            else
            {
                CD_ERROR("[SceneSimulationManager] Failed to launch standalone runtime");
            }
        }
        else
        {
            CD_INFO("Launching embedded runtime...");
            if (app)
            {
                editorScene = activeScene; // Backup

                // Load into LevelManager
                auto levelManager = Engine::Instance().GetService<ILevelManager>();
                if (levelManager)
                {
                    levelManager->LoadScene(tempPath);
                    activeScene = std::shared_ptr<GameScene>(&levelManager->GetGameScene(),
                                                             [](GameScene *) {});
                }

                activeScene = std::make_shared<GameScene>();
                SceneSerializer runtimeLoader(activeScene);
                runtimeLoader.DeserializeBinary(tempPath);

                // Load skybox for the deserialized scene
                SceneLoader().LoadSkyboxForScene(*activeScene);

                // Initialize collisions for the simulation
                if (levelManager)
                {
                    levelManager->InitCollisions();
                }

                // Clear registry before starting embedded simulation
                if (newScene)
                {
                    newScene->GetRegistry().clear();
                }

                // Spawn Player entity
                Vector3 spawnPos =
                    levelManager ? levelManager->GetSpawnPosition() : Vector3{0, 5, 0};
                CHD::RuntimeInitializer::InitializePlayer(newScene.get(), spawnPos, 0.15f);

                // Register scene in ECS Scene Manager for systems to access
                Engine::Instance().GetECSSceneManager().LoadScene(newScene);

                *runtimeLayer = new CHD::RuntimeLayer(newScene);
                app->PushLayer(*runtimeLayer);

                // Enable mouse capture for camera control in embedded simulation
                DisableCursor();
            }
        }
    }
}

void SceneSimulationManager::OnSceneStop(std::shared_ptr<GameScene> &activeScene,
                                         std::shared_ptr<GameScene> editorScene,
                                         std::shared_ptr<Scene> &newScene,
                                         CHD::RuntimeLayer **runtimeLayer, EngineApplication *app)
{
    m_SceneState = SceneState::Edit;
    CD_INFO("Scene Play stopped");

    if (m_RuntimeMode == RuntimeMode::Embedded)
    {
        if (*runtimeLayer != nullptr && app != nullptr)
        {
            app->PopLayer(*runtimeLayer);
            delete *runtimeLayer;
            *runtimeLayer = nullptr;
        }

        // Unload from ECS Scene Manager
        Engine::Instance().GetECSSceneManager().UnloadCurrentScene();

        // Restore mouse cursor for editor control
        EnableCursor();

        // Clear simulation entities
        if (newScene)
        {
            newScene->GetRegistry().clear();
        }

        // Restore editor scene
        if (editorScene)
            activeScene = editorScene;
    }
}
} // namespace CHEngine
