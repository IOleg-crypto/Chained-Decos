#include "runtime_application.h"
#include "core/log.h"
#include "engine/config/config_manager.h"
#include "engine/core/application/application.h"
#include "engine/core/engine_layer.h"
#include "engine/scene/core/scene_serializer.h"
#include "engine/scene/ecs/components/spawn_point_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include "logic/RuntimeInitializer.h"
#include "project/project.h"
#include "runtime_layer.h"
#include <filesystem>
#include <raylib.h>

using namespace CHEngine;

namespace CHD
{

RuntimeApplication::RuntimeApplication(int argc, char *argv[])
    : Application("Chained Decos"), m_isGameInitialized(false), m_cursorDisabled(false),
      m_showDebugCollision(false), m_showDebugStats(false)
{
    CD_INFO("[RuntimeApplication] Pre-initialization...");

    // 0. Push Engine Layer
    PushLayer(new EngineLayer());

    // Parse command line arguments
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);

    // Initial setup
    SetTraceLogLevel(LOG_INFO);

    ConfigManager configManager;
    bool configLoaded = false;
    std::string configPath = std::string(PROJECT_ROOT_DIR) + "/game.cfg";
    if (configManager.LoadFromFile(configPath))
    {
        CD_INFO("[RuntimeApplication] Loaded config from %s", configPath.c_str());
        configLoaded = true;
    }

    // 1. Determine which scene to load
    std::string sceneToLoad = m_gameConfig.mapPath;

    if (sceneToLoad.empty())
    {
        std::filesystem::path root(PROJECT_ROOT_DIR);
        std::shared_ptr<Project> project = nullptr;
        try
        {
            for (auto const &dir_entry : std::filesystem::recursive_directory_iterator(root))
            {
                if (dir_entry.path().extension() == ".chproject")
                {
                    project = Project::Load(dir_entry.path());
                    if (project)
                        break;
                }
            }
        }
        catch (...)
        {
        }

        if (project)
        {
            std::string startScene = project->GetConfig().startScene;
            if (!startScene.empty())
            {
                std::filesystem::path scenePath = project->GetProjectDirectory() / startScene;
                sceneToLoad = scenePath.string();
            }
        }
    }

    // 2. Initialize Core Systems
    InitInput();
    Audio::LoadSound("player_fall",
                     std::string(PROJECT_ROOT_DIR) + "/resources/audio/wind-gust_fall.wav");

    m_ActiveScene = std::make_shared<Scene>("RuntimeScene");
    SceneManager::LoadScene(m_ActiveScene);

    ModelLoader::LoadGameModels();

    if (!sceneToLoad.empty())
    {
        CD_INFO("[RuntimeApplication] Loading scene: %s", sceneToLoad.c_str());
        ECSSceneSerializer serializer(m_ActiveScene);
        if (serializer.Deserialize(sceneToLoad))
        {
            CD_INFO("[RuntimeApplication] Scene loaded successfully");
            m_isGameInitialized = true;
        }
        else
        {
            CD_ERROR("[RuntimeApplication] Failed to load scene: %s", sceneToLoad.c_str());
        }
    }

    // Find Spawn Position from ECS
    Vector3 spawnPos = {0, 5, 0};
    if (m_isGameInitialized)
    {
        auto spawnView =
            m_ActiveScene->GetRegistry().view<SpawnPointComponent, TransformComponent>();
        if (spawnView.begin() != spawnView.end())
        {
            spawnPos = spawnView.get<TransformComponent>(*spawnView.begin()).position;
            CD_INFO("[RuntimeApplication] Found Spawn Point in ECS at (%.2f, %.2f, %.2f)",
                    spawnPos.x, spawnPos.y, spawnPos.z);
        }
    }

    configManager.LoadFromFile(std::string(PROJECT_ROOT_DIR) + "/game.cfg");
    float sensitivity = configManager.GetMouseSensitivity();
    if (sensitivity <= 0.0f)
        sensitivity = 0.15f;

    m_playerEntity =
        CHD::RuntimeInitializer::InitializePlayer(m_ActiveScene.get(), spawnPos, sensitivity);

    Camera3D camera = {0};
    camera.position = {spawnPos.x, spawnPos.y + 5.0f, spawnPos.z + 10.0f};
    camera.target = spawnPos;
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Renderer::SetCamera(camera);

    // 3. Setup Layers
    PushLayer(new CHD::RuntimeLayer(m_ActiveScene));

    // Disable cursor for game control
    ::DisableCursor();

    CD_INFO("[RuntimeApplication] Game application initialized successfully.");
}

// Destructor is = default in header

void RuntimeApplication::OnEvent(CHEngine::Event &e)
{
    Application::OnEvent(e);
}

void RuntimeApplication::InitInput()
{
    Input::RegisterAction(KEY_F2,
                          [this]
                          {
                              m_showDebugCollision = !m_showDebugCollision;
                              CD_INFO("Debug Collision: %s", m_showDebugCollision ? "ON" : "OFF");
                          });

    Input::RegisterAction(KEY_F3,
                          [this]
                          {
                              m_showDebugStats = !m_showDebugStats;
                              CD_INFO("Debug Stats: %s", m_showDebugStats ? "ON" : "OFF");
                          });
}

} // namespace CHD

namespace CHEngine
{
Application *CreateApplication(int argc, char **argv)
{
    return new CHD::RuntimeApplication(argc, argv);
}
} // namespace CHEngine
