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
    config.fullscreen = m_gameConfig.fullscreen;
    config.vsync = true;
    config.enableAudio = true;
}

void RuntimeApplication::OnRegister()
{
    auto &engine = Engine::Instance();

    // Register LevelManager (Specialized for CHD)
    auto levelManager = std::make_shared<LevelManager>();
    engine.RegisterService<ILevelManager>(levelManager);

    CD_INFO("[RuntimeApplication] Game systems registered (LevelManager).");
}

void RuntimeApplication::OnStart()
{
    CD_INFO("[RuntimeApplication] Starting game...");

    // Initialize Static Singletons
    // Note: Renderer/Input/Audio are already initialized by Engine::Initialize

    Audio::LoadSound("player_fall",
                     std::string(PROJECT_ROOT_DIR) + "/resources/audio/wind-gust_fall.wav");

    // Initialize Scene System (new architecture)
    m_ActiveScene = std::make_shared<Scene>("RuntimeScene");
    CD_INFO("[RuntimeApplication] Created active scene: %s", m_ActiveScene->GetName().c_str());

    auto levelManager = Engine::Instance().GetService<ILevelManager>();
    if (levelManager)
        levelManager->SetActiveScene(m_ActiveScene);

    // Initial player state
    Vector3 spawnPos = {0, 2, 0};

    // Load mouse sensitivity from config
    ConfigManager configManager;
    configManager.LoadFromFile(std::string(PROJECT_ROOT_DIR) + "/game.cfg");
    float sensitivity = configManager.GetMouseSensitivity();
    if (sensitivity <= 0.0f)
        sensitivity = 0.15f; // Default if not set

    // Initialize player via Initializer
    m_playerEntity =
        CHD::RuntimeInitializer::InitializePlayer(m_ActiveScene.get(), spawnPos, sensitivity);

    CD_INFO("[RuntimeApplication] ECS Player entity created");

    // Initialize camera to follow player
    Camera3D camera = {0};
    camera.position = {spawnPos.x, spawnPos.y + 5.0f, spawnPos.z + 10.0f};
    camera.target = spawnPos;
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Renderer::SetCamera(camera);
    CD_INFO("[RuntimeApplication] Camera initialized at (%.2f, %.2f, %.2f)", camera.position.x,
            camera.position.y, camera.position.z);

    // Push RuntimeLayer using the new Layer system
    if (GetAppRunner())
    {
        GetAppRunner()->PushLayer(new CHD::RuntimeLayer(m_ActiveScene));
    }

    // Load initial map if provided (from editor or command line)
    if (!m_gameConfig.mapPath.empty())
    {
        CD_INFO("[RuntimeApplication] Loading scene from: %s", m_gameConfig.mapPath.c_str());
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager && levelManager->LoadScene(m_gameConfig.mapPath))
        {
            m_isGameInitialized = true;

            CD_INFO("[RuntimeApplication] Scene loaded successfully, game initialized");
        }
        else
        {
            CD_ERROR("[RuntimeApplication] Failed to load scene: %s", m_gameConfig.mapPath.c_str());
        }
    }
    // Initialize input
    InitInput();

    // Set window icon
    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/CHEngine.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(m_icon);
    UnloadImage(m_icon);

    CD_INFO("[RuntimeApplication] Game application initialized with ECS.");
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
