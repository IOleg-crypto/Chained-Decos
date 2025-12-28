#include "SceneSimulationManager.h"
#include "core/Base.h"
#include "core/Engine.h"
#include "core/application/EngineApplication.h"
#include "core/interfaces/ILevelManager.h"
#include "core/physics/Physics.h"
#include "editor/logic/SceneCloner.h"
#include "project/Runtime/RuntimeLayer.h"
#include "project/Runtime/logic/RuntimeInitializer.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/map/SceneSerializer.h"
#include <cstdlib>
#include <raylib.h>

namespace CHEngine
{
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
    if (activeScene)
    {
        std::string tempPath = SceneCloner::GetTempPath();
        if (tempPath.find(".json") != std::string::npos)
            tempPath.replace(tempPath.find(".json"), 5, ".chscene");

        // Ensure absolute path for standalone runtime
        std::filesystem::path absoluteTempPath = std::filesystem::absolute(tempPath);
        tempPath = absoluteTempPath.string();

        SceneSerializer serializer(activeScene);
        if (serializer.SerializeBinary(tempPath))
        {
            if (m_RuntimeMode == RuntimeMode::Standalone)
            {
                std::filesystem::path runtimePath =
                    std::filesystem::path(PROJECT_ROOT_DIR) / "build" / "bin" / "Runtime.exe";
                std::string cmd = TextFormat("start \"\" \"%s\" --map \"%s\" --skip-menu",
                                             runtimePath.string().c_str(), tempPath.c_str());
                CD_INFO("Launching standalone runtime: %s", cmd.c_str());
                std::system(cmd.c_str());
                CD_INFO("Standalone runtime process started");
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
