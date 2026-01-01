#include "RuntimeApplication.h"
#include "RuntimeLayer.h"
#include "core/Log.h"
#include "core/application/Application.h"
#include "core/audio/Audio.h"
#include "core/config/ConfigManager.h"
#include "core/input/Input.h"
#include "core/module/ModuleManager.h"
#include "core/renderer/Renderer.h"
#include "logic/RuntimeInitializer.h"
#include "project/Project.h"
#include "scene/main/LevelManager.h"
#include <raylib.h>

using namespace CHEngine;

namespace CHD
{

RuntimeApplication::RuntimeApplication(int argc, char *argv[])
    : Application("Chained Decos"), m_isGameInitialized(false), m_cursorDisabled(false),
      m_showDebugCollision(false), m_showDebugStats(false)
{
    CD_INFO("[RuntimeApplication] Pre-initialization...");

    // Parse command line arguments
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);

    // Initial setup logic moved from OnConfigure/OnStart
    SetTraceLogLevel(LOG_INFO);

    ConfigManager configManager;
    bool configLoaded = false;
    std::string configPath = std::string(PROJECT_ROOT_DIR) + "/game.cfg";
    if (configManager.LoadFromFile(configPath))
    {
        CD_INFO("[RuntimeApplication] Loaded config from %s", configPath.c_str());
        configLoaded = true;
    }

    int width = m_gameConfig.width;
    int height = m_gameConfig.height;

    if ((width == 1280 && height == 720) && configLoaded)
    {
        configManager.GetResolution(width, height);
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

    // 2. Initialize input, audio, and basic scene
    InitInput();
    Audio::LoadSound("player_fall",
                     std::string(PROJECT_ROOT_DIR) + "/resources/audio/wind-gust_fall.wav");

    m_ActiveScene = std::make_shared<Scene>("RuntimeScene");

    // Use static LevelManager
    LevelManager::SetActiveScene(m_ActiveScene);

    ModelLoader::LoadGameModels();

    if (!sceneToLoad.empty())
    {
        if (LevelManager::LoadScene(sceneToLoad))
        {
            LevelManager::RefreshMapEntities();
            LevelManager::RefreshUIEntities();
            m_isGameInitialized = true;
        }
    }

    Vector3 spawnPos = m_isGameInitialized ? LevelManager::GetSpawnPosition() : Vector3{0, 5, 0};

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

RuntimeApplication::~RuntimeApplication()
{
}

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
