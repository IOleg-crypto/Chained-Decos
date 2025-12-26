#include "RuntimeApplication.h"
#include "RuntimeLayer.h"
#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/assets/AssetManager.h"
#include "core/audio/Audio.h"
#include "core/input/Input.h"
#include "core/physics/Physics.h"
#include "core/renderer/Renderer.h"
#include "logic/RuntimeInitializer.h"
#include "scene/main/LevelManager.h"

using namespace CHEngine;

#include "core/config/ConfigManager.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"

#include "scene/resources/model/Model.h"
#include <raylib.h>
#include <rlImGui.h>

using CHEngine::MenuEvent;
using CHEngine::MenuEventType;
using namespace CHEngine;

namespace CHD
{

using MenuEventCallback = Menu::MenuEventCallback;

RuntimeApplication::RuntimeApplication(int argc, char *argv[])
    : m_showMenu(true), m_isGameInitialized(false), m_cursorDisabled(false),
      m_showDebugCollision(false), m_showDebugStats(false)
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

    // Register LevelManager
    auto levelManager = std::make_shared<LevelManager>();
    engine.RegisterService<ILevelManager>(levelManager);

    // Register WorldManager
    auto worldManager = std::make_shared<WorldManager>();
    engine.RegisterService<WorldManager>(worldManager);

    // Register CollisionManager
    auto collisionManager = std::make_shared<CollisionManager>();
    engine.RegisterService<CollisionManager>(collisionManager);

    CD_INFO("[RuntimeApplication] Game systems registered (LevelManager, WorldManager, "
            "CollisionManager).");
}

void RuntimeApplication::OnStart()
{
    CD_INFO("[RuntimeApplication] Starting game...");

    // Initialize Static Singletons
    // Note: Renderer/Input/Audio are already initialized by Engine::Initialize

    Audio::LoadSound("player_fall",
                     std::string(PROJECT_ROOT_DIR) + "/resources/audio/wind-gust_fall.wav");

    // Initialize Menu
    m_menu = std::make_shared<Menu>();
    Engine::Instance().RegisterService<IMenu>(m_menu);
    m_menu->Initialize(&Engine::Instance());
    m_menu->SetupStyle();

    // Register Menu Events
    m_menu->SetEventCallback(
        [this](const CHEngine::MenuEvent &event)
        {
            switch (event.GetMenuEventType())
            {
            case CHEngine::MenuEventType::StartGame:
            case CHEngine::MenuEventType::StartGameWithMap:
            {
                std::string mapName = event.GetMapName();
                if (mapName.empty())
                    mapName = m_menu->GetSelectedMapName();

                auto levelManager = Engine::Instance().GetService<ILevelManager>();
                if (levelManager && levelManager->LoadScene(mapName))
                {
                    m_isGameInitialized = true;
                    m_showMenu = false;
                }
                break;
            }
            case CHEngine::MenuEventType::ResumeGame:
            {
                if (m_isGameInitialized)
                {
                    m_showMenu = false;
                }
                break;
            }
            case CHEngine::MenuEventType::ExitGame:
            {
                Engine::Instance().RequestExit();
                break;
            }
            case CHEngine::MenuEventType::BackToMain:
            {
                // Internal menu state change handled by Menu class
                break;
            }
            default:
                break;
            }
        });

    CD_INFO("[RuntimeApplication] Menu initialized and events registered");

    // Initialize ECS
    REGISTRY.clear();

    // Initial player state
    Vector3 spawnPos = {0, 2, 0};

    // Load mouse sensitivity from config
    ConfigManager configManager;
    configManager.LoadFromFile(std::string(PROJECT_ROOT_DIR) + "/game.cfg");
    float sensitivity = configManager.GetMouseSensitivity();
    if (sensitivity <= 0.0f)
        sensitivity = 0.15f; // Default if not set

    // Initialize player via Initializer
    m_playerEntity = CHD::RuntimeInitializer::InitializePlayer(spawnPos, sensitivity);

    CD_INFO("[RuntimeApplication] ECS Player entity created");

    // Apply visual offset to player render component
    if (REGISTRY.valid(m_playerEntity) && REGISTRY.all_of<RenderComponent>(m_playerEntity))
    {
        auto &renderComp = REGISTRY.get<RenderComponent>(m_playerEntity);
        renderComp.offset = {0.0f, Player::MODEL_Y_OFFSET, 0.0f};
        CD_INFO("[RuntimeApplication] Set player visual offset to (0, %.2f, 0)",
                Player::MODEL_Y_OFFSET);
    }

    // Initialize camera to follow player (Hazel-style)
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
        GetAppRunner()->PushLayer(new CHD::RuntimeLayer());
    }

    // Load initial map if provided (from editor or command line)
    if (!m_gameConfig.mapPath.empty())
    {
        CD_INFO("[RuntimeApplication] Loading scene from: %s", m_gameConfig.mapPath.c_str());
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager && levelManager->LoadScene(m_gameConfig.mapPath))
        {
            m_isGameInitialized = true;
            m_showMenu = false; // Always skip menu when loading from editor
            CD_INFO("[RuntimeApplication] Scene loaded successfully, game initialized");
        }
        else
        {
            CD_ERROR("[RuntimeApplication] Failed to load scene: %s", m_gameConfig.mapPath.c_str());
            m_showMenu = true; // Show menu on failure
        }
    }
    else
    {
        // No map provided - show menu unless skipMenu flag is set
        m_showMenu = !m_gameConfig.skipMenu;
        CD_INFO("[RuntimeApplication] No map provided, showing menu: %s",
                m_showMenu ? "yes" : "no");
    }

    // Initialize cursor state
    m_cursorDisabled = !m_showMenu;
    if (m_cursorDisabled)
        DisableCursor();
    else
        EnableCursor();

    // Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

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
    // Update Input handled by Engine
    // Update Audio looping
    // (In future these will be in Engine::Update)

    // Get Menu through Engine (Legacy access)
    auto engine = &Engine::Instance();
    if (!engine)
        return;

    Menu *menu = m_menu.get();

    // Only handle console toggle here if we are NOT in the menu.
    // When in menu, Menu::HandleKeyboardNavigation handles it to avoid double-toggling.
    if (!m_showMenu && Input::IsKeyPressed(KEY_GRAVE) && menu)
    {
        menu->ToggleConsole();
    }

    // Manage cursor visibility based on menu state
    if (m_showMenu)
    {
        // Menu is open - show system cursor
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.MouseDrawCursor = false;

        if (m_cursorDisabled)
        {
            EnableCursor();
            m_cursorDisabled = false;
        }

        // Menu actions are now handled via event callbacks registered in OnStart()
    }
    else
    {
        // Menu is closed - disable keyboard navigation
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        if (io.NavActive)
        {
            io.NavActive = false;
            io.NavVisible = false;
        }

        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;

        // Game is running
        if (m_isGameInitialized)
        {
            bool consoleOpen =
                menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen();

            if (consoleOpen)
            {
                if (m_cursorDisabled)
                {
                    EnableCursor();
                    m_cursorDisabled = false;
                }
            }
            else
            {
                if (!m_cursorDisabled)
                {
                    DisableCursor();
                    m_cursorDisabled = true;
                }

                // ECS Systems are now handled by RuntimeLayer
            }
        }
        else
        {
            if (m_cursorDisabled)
            {
                EnableCursor();
                m_cursorDisabled = false;
            }
        }
    }
}

void RuntimeApplication::OnRender()
{
    // Frame is begun by EngineApplication::Render()

    // Get Menu
    auto *engine = &Engine::Instance();
    Menu *menu = m_menu.get();

    if (m_showMenu && menu)
    {
        rlImGuiBegin();

        // Render the menu
        menu->Render();

        if (menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
        {
            menu->GetConsoleManager()->RenderConsole();
        }
        rlImGuiEnd();
    }
    else
    {
        if (m_isGameInitialized)
        {
            // 3D Rendering (this part is fine for now, though could be moved to GameLayer)
            Camera3D &camera = Renderer::GetCamera();
            Renderer::BeginScene(camera);

            auto models = Engine::Instance().GetService<ModelLoader>();
            if (models)
                models->DrawAllModels();

            auto levelManager = Engine::Instance().GetService<ILevelManager>();
            if (levelManager)
                levelManager->RenderEditorMap();

            auto world = Engine::Instance().GetService<WorldManager>();
            if (world)
                world->Render();

            Renderer::EndScene();

            // HUD is now rendered by GameLayer::RenderUI
        }
    }
    // Render console in game - Now handled inside menu->Render() when showMenu is true
    // If we want it while playing, we need a single rlImGuiBegin/End block per frame
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu)
        {
            rlImGuiBegin();
            menu->Render(); // Calling Render here instead of just Console so screens can handle it
                            // if needed
            rlImGuiEnd();
        }
    }

    // Debug Stats
    if (m_showDebugStats)
    {
        DrawFPS(10, 10);

        if (m_isGameInitialized)
        {
            // Draw player position for debugging
            if (REGISTRY.valid(m_playerEntity))
            {
                auto &transform = REGISTRY.get<TransformComponent>(m_playerEntity);
                DrawText(TextFormat("Pos: %.2f, %.2f, %.2f", transform.position.x,
                                    transform.position.y, transform.position.z),
                         10, 30, 20, GREEN);
            }
        }
    }

    // Frame is ended by EngineApplication::Render()
}

void RuntimeApplication::OnShutdown()
{
    CD_INFO("[RuntimeApplication] Cleaning up game resources...");

    // Clear ECS
    REGISTRY.clear();

    // Shutdown Managers
    // (In future these will be in Engine::Shutdown)

    auto collisionManager = Engine::Instance().GetService<CollisionManager>();
    if (collisionManager && !collisionManager->GetColliders().empty())
    {
        collisionManager->ClearColliders();
        {
            if (m_menu)
            {
                m_menu->SetGameInProgress(false);
            }
        }
    }

    CD_INFO("[RuntimeApplication] Game resources cleaned up successfully");
}

void RuntimeApplication::InitInput()
{
    CD_INFO("[RuntimeApplication] Setting up game-specific input bindings...");

    auto &engine = Engine::Instance();
    auto *menu = m_menu.get();

    if (!menu)
    {
        CD_WARN("[RuntimeApplication] Menu not found, skipping input bindings");
        return;
    }

    Input::RegisterAction(KEY_F1,
                          [this, menu]
                          {
                              if (!m_showMenu && m_isGameInitialized)
                              {
                                  menu->SetGameInProgress(true);
                                  m_showMenu = true;
                                  EnableCursor(); // Show system cursor when opening menu
                              }
                          });

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
    if (m_menu && m_showMenu)
    {
        m_menu->OnEvent(e);
    }
}

} // namespace CHD
