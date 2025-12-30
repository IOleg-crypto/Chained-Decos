#include "RuntimeApplication.h"
#include "RuntimeLayer.h"
#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/audio/Audio.h"
#include "core/config/ConfigManager.h"
#include "core/input/Input.h"
#include "core/interfaces/ILevelManager.h"
#include "core/renderer/Renderer.h"
#include "logic/RuntimeInitializer.h"
#include "project/Project.h"
#include "scene/main/LevelManager.h"
#include <raylib.h>
#include <rlImGui.h>

using namespace CHEngine;

namespace CHD
{

RuntimeApplication::RuntimeApplication(int argc, char *argv[])
    : m_isGameInitialized(false), m_cursorDisabled(false), m_showDebugCollision(false),
      m_showDebugStats(false)
{
    // Parse command line arguments
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);
}

RuntimeApplication::~RuntimeApplication()
{
    CD_INFO("RuntimeApplication destructor called.");
}

void RuntimeApplication::OnConfigure(IApplication::EngineConfig &config)
{
    CD_INFO("[RuntimeApplication] Pre-initialization...");
    SetTraceLogLevel(LOG_INFO);

    // Load config from game.cfg BEFORE setting window size
    ConfigManager configManager;
    bool configLoaded = false;

    // Try loading from game.cfg using PROJECT_ROOT_DIR (where the game will be installed)
    std::string configPath = std::string(PROJECT_ROOT_DIR) + "/game.cfg";
    if (configManager.LoadFromFile(configPath))
    {
        CD_INFO("[RuntimeApplication] Loaded config from %s", configPath.c_str());
        configLoaded = true;
    }
    else
    {
        CD_WARN("[RuntimeApplication] Could not load game.cfg, using defaults");
    }

    // Get resolution from config (if not specified in command line)
    int width = m_gameConfig.width;
    int height = m_gameConfig.height;

    // If resolution not specified in command line (using defaults)
    // and config loaded, use values from config
    if ((width == 1280 && height == 720) && configLoaded)
    {
        configManager.GetResolution(width, height);
        CD_INFO("[RuntimeApplication] Using resolution from config: %dx%d", width, height);
    }

    // Also check fullscreen from config
    if (configLoaded && !m_gameConfig.fullscreen)
    {
        m_gameConfig.fullscreen = configManager.IsFullscreen();
    }

    if (m_gameConfig.developer)
    {
        CommandLineHandler::ShowConfig(m_gameConfig);
    }

    CD_INFO("[RuntimeApplication] Window config: %dx%d (fullscreen: %s)", width, height,
            m_gameConfig.fullscreen ? "yes" : "no");

    // Update EngineConfig
    config.width = width;
    config.height = height;
    config.title = "Chained Decos";
    config.fullscreen = m_gameConfig.fullscreen = false;
    config.vsync = true;
    config.enableAudio = true;
}

void RuntimeApplication::OnRegister()
{
    auto &engine = Engine::Instance();

    // Register LevelManager as a module (Specialized for CHD)
    // It will register itself as ILevelManager service during initialization
    engine.RegisterModule(std::make_unique<LevelManager>());

    CD_INFO("[RuntimeApplication] Game systems registered (LevelManager).");
}

void RuntimeApplication::OnStart()
{
    CD_INFO("[RuntimeApplication] Pre-initialization...");

    // 1. Determine which scene to load
    std::string sceneToLoad = m_gameConfig.mapPath;

    // If no scene provided via command line, search for project file
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
                    CD_INFO("[RuntimeApplication] Found project file: %s",
                            dir_entry.path().string().c_str());
                    project = Project::Load(dir_entry.path());
                    if (project)
                        break;
                }
            }
        }
        catch (...)
        {
            CD_WARN("[RuntimeApplication] Error while searching for project file");
        }

        if (project)
        {
            std::string startScene = project->GetConfig().startScene;
            if (!startScene.empty())
            {
                std::filesystem::path scenePath = project->GetProjectDirectory() / startScene;
                sceneToLoad = scenePath.string();
                CD_INFO("[RuntimeApplication] Loading start_scene from project: %s",
                        sceneToLoad.c_str());
            }
        }
    }

    // 2. Initialize input, audio, and basic scene
    InitInput();
    Audio::LoadSound("player_fall",
                     std::string(PROJECT_ROOT_DIR) + "/resources/audio/wind-gust_fall.wav");

    m_ActiveScene = std::make_shared<Scene>("RuntimeScene");
    auto levelManager = Engine::Instance().GetService<ILevelManager>();
    if (levelManager)
    {
        levelManager->SetActiveScene(m_ActiveScene);

        // Pre-load all game models so they are available for the scene entities
        if (auto modelLoader = std::dynamic_pointer_cast<ModelLoader>(
                Engine::Instance().GetService<IModelLoader>()))
        {
            modelLoader->LoadGameModels();
        }

        // 3. Load the scene content BEFORE spawning player or pushing layers
        if (!sceneToLoad.empty())
        {
            CD_INFO("[RuntimeApplication] Loading scene: %s", sceneToLoad.c_str());
            if (levelManager->LoadScene(sceneToLoad))
            {
                // Sync legacy objects to ECS entities
                levelManager->RefreshMapEntities();
                levelManager->RefreshUIEntities();
                m_isGameInitialized = true;
            }
            else
            {
                CD_ERROR("[RuntimeApplication] Failed to load scene: %s", sceneToLoad.c_str());
            }
        }
    }

    // 4. Initialize player and camera in the loaded world
    // Use spawn position from level manager if available
    Vector3 spawnPos =
        (levelManager && m_isGameInitialized) ? levelManager->GetSpawnPosition() : Vector3{0, 5, 0};

    // Load mouse sensitivity from config
    ConfigManager configManager;
    configManager.LoadFromFile(std::string(PROJECT_ROOT_DIR) + "/game.cfg");
    float sensitivity = configManager.GetMouseSensitivity();
    if (sensitivity <= 0.0f)
        sensitivity = 0.15f;

    m_playerEntity =
        CHD::RuntimeInitializer::InitializePlayer(m_ActiveScene.get(), spawnPos, sensitivity);
    CD_INFO("[RuntimeApplication] ECS Player entity created at (%.2f, %.2f, %.2f)", spawnPos.x,
            spawnPos.y, spawnPos.z);

    // Initial camera setup
    Camera3D camera = {0};
    camera.position = {spawnPos.x, spawnPos.y + 5.0f, spawnPos.z + 10.0f};
    camera.target = spawnPos;
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Renderer::SetCamera(camera);

    // 5. Setup Viewport and Layers
    if (GetAppRunner())
    {
        GetAppRunner()->PushLayer(new CHD::RuntimeLayer(m_ActiveScene));
    }

    // Set window icon
    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    if (m_icon.data != nullptr)
    {
        ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(m_icon);
        UnloadImage(m_icon);
    }

    // Disable cursor for game control (mouse delta camera movement)
    Engine::Instance().GetInputManager().DisableCursor();

    CD_INFO("[RuntimeApplication] Game application initialized successfully.");
}

void RuntimeApplication::OnUpdate(float deltaTime)
{
    // Application-level update logic
    // Game logic is handled by RuntimeLayer and ECS systems
}

void RuntimeApplication::OnRender()
{
    // Frame is begun by EngineApplication::Render()
    // Rendering is handled by RuntimeLayer
    // Frame is ended by EngineApplication::Render()
}

void RuntimeApplication::OnShutdown()
{
    CD_INFO("[RuntimeApplication] Cleaning up game resources...");

    // Clear ECS (Scene will be destroyed automatically as it's a shared_ptr)

    // Shutdown Managers
    // (In future these will be in Engine::Shutdown)

    CD_INFO("[RuntimeApplication] Game resources cleaned up successfully");
}

void RuntimeApplication::InitInput()
{
    CD_INFO("[RuntimeApplication] Setting up game-specific input bindings...");

    // Debug toggles
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

    CD_INFO("[RuntimeApplication] Game input bindings configured.");
}

void RuntimeApplication::OnEvent(CHEngine::Event &e)
{
    // Event handling delegated to RuntimeLayer
}

} // namespace CHD
