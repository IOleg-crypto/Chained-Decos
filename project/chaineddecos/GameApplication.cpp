#include "GameApplication.h"
#include "Systems/MapSystem/LevelManager.h"
// #include "Systems/PlayerSystem/PlayerController.h"
// #include "Systems/RenderingSystem/RenderingSystem.h"
#include "Systems/UIController/UIManager.h"
#include "core/config/Core/ConfigManager.h"
#include "core/ecs/Examples.h"
#include "core/engine/EngineApplication.h"
#include "core/object/module/Core/ModuleManager.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/main/Core/World.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/input/Core/InputManager.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include "servers/rendering/Core/RenderManager.h"

#include "imgui.h"
#include "rlImGui.h"
#include <GLFW/glfw3.h>
#include <raylib.h>

GameApplication::GameApplication(int argc, char *argv[])
    : m_showMenu(true), m_isGameInitialized(false), m_cursorDisabled(false)
{
    // Parse command line arguments
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);
}

GameApplication::~GameApplication()
{
    TraceLog(LOG_INFO, "GameApplication destructor called.");
}

void GameApplication::OnConfigure(EngineConfig &config)
{
    TraceLog(LOG_INFO, "[GameApplication] Pre-initialization...");
    SetTraceLogLevel(LOG_INFO);

    // Load config from game.cfg BEFORE setting window size
    ConfigManager configManager;
    bool configLoaded = false;

    // Try loading from game.cfg using PROJECT_ROOT_DIR (where the game will be installed)
    std::string configPath = std::string(PROJECT_ROOT_DIR) + "/game.cfg";
    if (configManager.LoadFromFile(configPath))
    {
        TraceLog(LOG_INFO, "[GameApplication] Loaded config from %s", configPath.c_str());
        configLoaded = true;
    }
    else
    {
        TraceLog(LOG_WARNING, "[GameApplication] Could not load game.cfg, using defaults");
    }

    // Get resolution from config (if not specified in command line)
    int width = m_gameConfig.width;
    int height = m_gameConfig.height;

    // If resolution not specified in command line (using defaults)
    // and config loaded, use values from config
    if ((width == 1280 && height == 720) && configLoaded)
    {
        configManager.GetResolution(width, height);
        TraceLog(LOG_INFO, "[GameApplication] Using resolution from config: %dx%d", width, height);
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

    TraceLog(LOG_INFO, "[GameApplication] Window config: %dx%d (fullscreen: %s)", width, height,
             m_gameConfig.fullscreen ? "yes" : "no");

    // Update EngineConfig
    config.width = width;
    config.height = height;
    config.title = "Chained Decos";
    config.fullscreen = m_gameConfig.fullscreen;
    config.vsync = true;
    config.enableAudio = true;
}

void GameApplication::OnRegister()
{
    TraceLog(LOG_INFO, "[GameApplication] Registering services and modules...");

    auto &engine = Engine::Instance();

    // 1. Initialize Core Services (Legacy - to be removed later)
    m_collisionManager = std::make_shared<CollisionManager>();
    m_models = std::make_shared<ModelLoader>();
    m_world = std::make_shared<WorldManager>();

    // Register core services
    engine.RegisterService<CollisionManager>(m_collisionManager);
    engine.RegisterService<ModelLoader>(m_models);
    engine.RegisterService<WorldManager>(m_world);

    TraceLog(LOG_INFO, "[GameApplication] Core engine services registered.");

    // 2. Register Game Systems (Modules)
    // Legacy modules - keeping LevelManager and UIManager for now
    engine.RegisterModule(std::make_unique<LevelManager>());
    engine.RegisterModule(std::make_unique<UIManager>());

    // PlayerController and RenderingSystem are replaced by ECS Systems
    // engine.RegisterModule(std::make_unique<PlayerController>());
    // engine.RegisterModule(std::make_unique<RenderingSystem>());

    TraceLog(LOG_INFO, "[GameApplication] Game systems registered.");
}

void GameApplication::OnStart()
{
    TraceLog(LOG_INFO, "[GameApplication] Starting game...");

    // Initialize Static Singletons
    if (AudioManager::Get().Initialize())
    {
        TraceLog(LOG_INFO, "[GameApplication] AudioManager initialized");
        AudioManager::Get().LoadSound(
            "player_fall", "D:\\gitnext\\Chained Decos\\resources\\audio\\wind-gust_fall.wav");
    }

    InputManager::Get().Initialize();

    // Initialize RenderManager with config
    RenderManager::Get().Initialize(m_gameConfig.width, m_gameConfig.height, "Chained Decos");

    // Setup ImGui style for menu (after ImGui is initialized by RenderManager)
    auto *engine = &Engine::Instance();
    if (engine->GetModuleManager())
    {
        auto *uiModule = engine->GetModuleManager()->GetModule("UI");
        if (uiModule)
        {
            UIManager *uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager && uiManager->GetMenu())
            {
                uiManager->GetMenu()->SetupStyle();
                TraceLog(LOG_INFO, "[GameApplication] ImGui style configured");
            }
        }
    }

    // Initialize ECS
    REGISTRY.clear();

    // Create Player Entity
    m_playerEntity = ECSExamples::CreatePlayer(Vector3{0, 2, 0});
    TraceLog(LOG_INFO, "[GameApplication] ECS Player entity created");

    // Initial state - show menu
    m_showMenu = true;

    // Initialize cursor state
    m_cursorDisabled = false;
    EnableCursor();

    // Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

    // Initialize input
    InitInput();

    // Game is not initialized until a map is selected
    m_isGameInitialized = false;

    // Set window icon
    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(m_icon);
    UnloadImage(m_icon);

    // Apply fullscreen
    /*
    if (m_gameConfig.fullscreen && !IsWindowFullscreen())
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }
    */

    TraceLog(LOG_INFO, "[GameApplication] Game application initialized with ECS.");
}

void GameApplication::OnUpdate(float deltaTime)
{
    // Update Input
    InputManager::Get().Update(deltaTime);

    // Get Menu through Engine (Legacy access)
    auto engine = &Engine::Instance();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    UIManager *uiManager = nullptr;
    Menu *menu = nullptr;

    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
            {
                menu = uiManager->GetMenu();
            }
        }
    }

    if (InputManager::Get().IsKeyPressed(KEY_GRAVE) && menu)
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

        HandleMenuActions();
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

                // ECS Update Loop
                PlayerSystem::Update(deltaTime);
                MovementSystem::Update(deltaTime);
                CollisionSystem::Update();
                LifetimeSystem::Update(deltaTime);
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

void GameApplication::OnRender()
{
    // Frame is begun by EngineApplication::Render()

    // Get Menu
    auto *engine = &Engine::Instance();
    // auto moduleManager = engine->GetModuleManager();
    UIManager *uiManager = nullptr;
    Menu *menu = nullptr;

    if (engine->GetModuleManager())
    {
        auto *uiModule = engine->GetModuleManager()->GetModule("UI");
        if (uiModule)
        {
            uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
                menu = uiManager->GetMenu();
        }
    }

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
            // 3D Rendering
            auto &camera = RenderManager::Get().GetCamera();
            RenderManager::Get().BeginMode3D(camera);

            // Render ECS entities
            RenderSystem::Render();

            // Render World (Legacy)
            if (m_world)
                m_world->Render();

            RenderManager::Get().EndMode3D();
        }
    }

    // Render console in game
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu)
        {
            rlImGuiBegin();
            menu->GetConsoleManager()->RenderConsole();
            rlImGuiEnd();
        }
    }

    // Frame is ended by EngineApplication::Render()
}

void GameApplication::OnShutdown()
{
    TraceLog(LOG_INFO, "[GameApplication] Cleaning up game resources...");

    // Clear ECS
    REGISTRY.clear();

    // Shutdown Managers
    RenderManager::Get().Shutdown();
    InputManager::Get().Shutdown();
    // AudioManager::Get().Shutdown(); // Optional, destructor handles it

    if (m_collisionManager && !m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        {
            auto *uiModule = ENGINE.GetModuleManager()->GetModule("UI");
            if (uiModule)
            {
                auto *uiManager = dynamic_cast<UIManager *>(uiModule);
                if (uiManager && uiManager->GetMenu())
                {
                    uiManager->GetMenu()->SetGameInProgress(false);
                }
            }
        }
    }

    TraceLog(LOG_INFO, "[GameApplication] Game resources cleaned up successfully");
}

void GameApplication::InitInput()
{
    TraceLog(LOG_INFO, "[GameApplication] Setting up game-specific input bindings...");

    auto *engine = &Engine::Instance();
    if (!engine)
    {
        TraceLog(LOG_WARNING, "[GameApplication] No engine provided, skipping input bindings");
        return;
    }

    // Get Menu through UIController
    auto moduleManager = engine->GetModuleManager();
    Menu *menu = nullptr;
    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            auto *uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
            {
                menu = uiManager->GetMenu();
            }
        }
    }

    if (!menu)
    {
        TraceLog(LOG_WARNING, "[GameApplication] Menu not found, skipping input bindings");
        return;
    }

    engine->GetInputManager()->RegisterAction(
        KEY_F1,
        [this, menu]
        {
            if (!m_showMenu && m_isGameInitialized)
            {
                SaveGameState();
                menu->SetGameInProgress(true);
                m_showMenu = true;
                EnableCursor(); // Show system cursor when opening menu
            }
        });

    engine->GetInputManager()->RegisterAction(
        KEY_ESCAPE,
        [this, menu]
        {
            if (!m_showMenu && m_isGameInitialized)
            {
                SaveGameState();
                menu->ResetAction();
                menu->SetGameInProgress(true);
                m_showMenu = true;
                EnableCursor(); // Show system cursor when opening menu
            }
        });
    TraceLog(LOG_INFO, "[GameApplication] Game input bindings configured.");
}

void GameApplication::HandleMenuActions()
{
    // Get UIManager through ModuleManager
    auto *engine = &Engine::Instance();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    if (!moduleManager)
        return;

    auto *uiModule = moduleManager->GetModule("UI");
    if (!uiModule)
        return;

    auto *uiManager = dynamic_cast<UIManager *>(uiModule);
    if (uiManager)
    {
        uiManager->HandleMenuActions(&m_showMenu, &m_isGameInitialized);
    }
}

void GameApplication::SaveGameState()
{
    /*
    if (!m_isGameInitialized)
    {
        return; // No state to save if game is not initialized
    }

    // Get PlayerController through Engine
    // Get PlayerController through ModuleManager
    auto *engine = &Engine::Instance();
    if (!engine || !engine->GetModuleManager())
        return;

    auto *playerModule = engine->GetModuleManager()->GetModule("Player");
    auto *playerController = dynamic_cast<PlayerController *>(playerModule);

    if (!playerController)
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - PlayerController not available");
        return;
    }

    // Get LevelManager through Engine
    auto levelManager = engine->GetLevelManager();
    if (!levelManager)
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - LevelManager not available");
        return;
    }

    // Get current map path
    std::string currentMapPath = levelManager->GetCurrentMapPath();

    if (currentMapPath.empty())
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - Current map path is empty");
        return;
    }

    playerController->SavePlayerState(currentMapPath);
    TraceLog(LOG_INFO, "[GameApplication] Game state saved (map: %s)", currentMapPath.c_str());
    */
}
