#include "scene_simulation_manager.h"
#include "components/physics/collision/core/physics.h"
#include "core/application/application.h"
#include "core/utils/base.h"
#include "editor/logic/scene_cloner.h"
#include "editor/utils/process_utils.h"
#include "project/project.h"
#include "runtime/logic/RuntimeInitializer.h"
#include "runtime/runtime_layer.h"
#include "scene/core/scene_manager.h"
#include "scene/core/scene_serializer.h"
#include "scene/ecs/components/spawn_point_component.h"
#include "scene/ecs/components/transform_component.h"
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

void SceneSimulationManager::OnScenePlay(Scene *newScene, CHD::RuntimeLayer **runtimeLayer,
                                         Application *app)
{
    m_SceneState = SceneState::Play;

    // 1. Find Spawn Point from ECS
    Vector3 spawnPos = {0, 5, 0};
    auto spawnView = newScene->GetRegistry().view<SpawnPointComponent, TransformComponent>();
    if (spawnView.begin() != spawnView.end())
    {
        spawnPos = spawnView.get<TransformComponent>(*spawnView.begin()).position;
        CD_INFO("Found Spawn Point in ECS at (%.2f, %.2f, %.2f)", spawnPos.x, spawnPos.y,
                spawnPos.z);
    }

    // 2. Determine project paths
    std::string tempPath;
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
            std::filesystem::create_directories(sceneDir);
        tempPath = (sceneDir / "test.chscene").string();
    }
    else
    {
        tempPath = (root / "test.chscene").string();
    }

    CD_INFO("[SceneSimulationManager] Saving runtime scene to: %s", tempPath.c_str());

    // 3. Serialize scene for simulation
    auto sharedScene = std::shared_ptr<Scene>(newScene, [](Scene *) {});
    ECSSceneSerializer serializer(sharedScene);
    serializer.Serialize(tempPath);

    if (m_RuntimeMode == RuntimeMode::Standalone)
    {
        std::filesystem::path runtimePath =
            std::filesystem::path(PROJECT_ROOT_DIR) / "build" / "bin" / "Runtime.exe";

        std::string commandLine =
            "\"" + runtimePath.string() + "\" --map \"" + tempPath + "\" --skip-menu";

        CD_INFO("[SceneSimulationManager] Launching standalone runtime: %s", commandLine.c_str());

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
            // Spawn Player entity
            CHD::RuntimeInitializer::InitializePlayer(newScene, spawnPos, 0.15f);

            // Register scene in ECS Scene Manager for systems to access
            if (SceneManager::IsInitialized())
            {
                auto sharedNew = std::shared_ptr<Scene>(newScene, [](Scene *) {});
                SceneManager::LoadScene(sharedNew);
            }

            auto sharedNew = std::shared_ptr<Scene>(newScene, [](Scene *) {});
            *runtimeLayer = new CHD::RuntimeLayer(sharedNew);
            app->PushLayer(*runtimeLayer);

            DisableCursor();
        }
    }
}

void SceneSimulationManager::OnSceneStop(Scene *newScene, CHD::RuntimeLayer **runtimeLayer,
                                         Application *app)
{
    m_SceneState = SceneState::Edit;
    CD_INFO("Scene Play stopped");

    if (m_RuntimeMode == RuntimeMode::Embedded)
    {
        if (*runtimeLayer != nullptr && app != nullptr)
        {
            app->PopLayer(*runtimeLayer);
            *runtimeLayer = nullptr;
        }

        if (SceneManager::IsInitialized())
        {
            SceneManager::UnloadCurrentScene();
        }

        EnableCursor();

        if (newScene)
        {
            newScene->GetRegistry().clear();
        }
    }
}
} // namespace CHEngine
