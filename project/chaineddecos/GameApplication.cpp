#include "GameApplication.h"
#include "Systems/MapSystem/MapSystem.h"
#include "Systems/PlayerSystem/PlayerSystem.h"
#include "Systems/RenderingSystem/RenderingSystem.h"
#include "Systems/UIController/UIController.h"
#include "core/config/Core/ConfigManager.h"
#include "core/engine/EngineApplication.h"
#include "core/object/module/Core/ModuleManager.h"
#include "project/chaineddecos/Menu/Console/ConsoleManagerHelpers.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/main/Core/World.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
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

    // Try loading from game.cfg (relative to build/bin directory)
    if (configManager.LoadFromFile("../../game.cfg"))
    {
        TraceLog(LOG_INFO, "[GameApplication] Loaded config from ../../game.cfg");
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

    auto engine = GetEngine();
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[GameApplication] Engine is null");
        return;
    }

    // 1. Initialize Core Services
    m_collisionManager = std::make_shared<CollisionManager>();
    m_models = std::make_shared<ModelLoader>();
    m_world = std::make_shared<WorldManager>();
    m_soundSystem = std::make_shared<AudioManager>();

    if (m_soundSystem->Initialize())
    {
        TraceLog(LOG_INFO, "[GameApplication] AudioManager initialized successfully.");
        // Preload fall sound
        if (m_soundSystem->LoadSound(
                "player_fall", "D:\\gitnext\\Chained Decos\\resources\\audio\\wind-gust_fall.wav"))
        {
            TraceLog(LOG_INFO, "[GameApplication] Fall sound loaded successfully.");
        }
        else
        {
            TraceLog(LOG_ERROR, "[GameApplication] Failed to load fall sound.");
        }
    }
    else
    {
        TraceLog(LOG_ERROR, "[GameApplication] AudioManager failed to initialize.");
    }

    // Register core services
    engine->RegisterService<CollisionManager>(m_collisionManager);
    engine->RegisterService<ModelLoader>(m_models);
    engine->RegisterService<WorldManager>(m_world);
    engine->RegisterService<AudioManager>(m_soundSystem);

    TraceLog(LOG_INFO, "[GameApplication] Core engine services registered.");

    // 2. Register Game Systems (Modules)
    // Register systems in dependency order:
    // 1. MapSystem (base, no dependencies on other game systems)
    // 2. UIController (also base)
    // 3. PlayerSystem (depends on MapSystem)
    // 4. RenderingSystem (depends on PlayerSystem and MapSystem)
    engine->RegisterModule(std::make_unique<MapSystem>());
    engine->RegisterModule(std::make_unique<UIController>());
    engine->RegisterModule(std::make_unique<PlayerSystem>());
    engine->RegisterModule(std::make_unique<RenderingSystem>());

    TraceLog(LOG_INFO, "[GameApplication] Game systems registered.");
}

void GameApplication::OnStart()
{
    TraceLog(LOG_INFO, "[GameApplication] Starting game...");

    // Initial state - show menu
    m_showMenu = true;

    // Initialize cursor state - menu is shown, so cursor should be enabled
    m_cursorDisabled = false;

    // Ensure cursor is visible when menu is shown (using system cursor)
    EnableCursor();

    // Configure ImGui for proper mouse/cursor handling in menu
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation
    io.MouseDrawCursor = false;                           // Use system cursor, not ImGui cursor

    // Dependency Injection: update ConsoleManager providers after all services are registered
    UpdateConsoleManagerProviders(GetEngine());

    // Initialize input after everything is ready
    InitInput();

    // Game is not initialized until a map is selected
    m_isGameInitialized = false;

    // Set window icon
    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(m_icon);
    UnloadImage(m_icon);

    // Apply fullscreen from config if needed
    if (m_gameConfig.fullscreen && !IsWindowFullscreen())
    {
        TraceLog(LOG_INFO, "[GameApplication] Setting fullscreen mode from config");
        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    TraceLog(LOG_INFO, "[GameApplication] Game application initialized (player will be initialized "
                       "when map is selected).");
}

void GameApplication::OnUpdate(float deltaTime)
{
    (void)deltaTime; // Unused for now

    // Get Menu through Engine
    // Note: MenuService was removed, accessing Menu via UIController or directly if registered
    // Wait, MenuService was removed. How do we access Menu?
    // UIController manages Menu.
    // Let's get UIController module.

    auto engine = GetEngine();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    UIController *uiController = nullptr;
    Menu *menu = nullptr;

    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            uiController = dynamic_cast<UIController *>(uiModule);
            if (uiController)
            {
                menu = uiController->GetMenu();
            }
        }
    }

    if (IsKeyPressed(KEY_GRAVE) && menu)
    {
        menu->ToggleConsole();
    }

    // Manage cursor visibility based on menu state
    if (m_showMenu)
    {
        // Menu is open - show system cursor (more reliable than ImGui cursor)
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Disable ImGui cursor drawing - use system cursor instead
        io.MouseDrawCursor = false;

        // Enable system cursor for menu interaction (only if currently disabled)
        if (m_cursorDisabled)
        {
            EnableCursor();
            m_cursorDisabled = false;
        }

        HandleMenuActions();
    }
    else
    {
        // Menu is closed - disable keyboard navigation to allow game input
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        // Force clear ImGui navigation state when menu closes to prevent input blocking
        if (io.NavActive)
        {
            io.NavActive = false;
            io.NavVisible = false;
        }

        // Also clear WantCaptureKeyboard and WantCaptureMouse to ensure player input isn't blocked
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;

        // Game is running - check console state
        if (m_isGameInitialized)
        {
            // Game is running - check console state
            bool consoleOpen =
                menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen();

            if (consoleOpen)
            {
                // Console is open - show system cursor (only if currently disabled)
                if (m_cursorDisabled)
                {
                    EnableCursor();
                    m_cursorDisabled = false;
                }
            }
            else
            {
                // Game is running, no console - hide cursor (only if currently enabled)
                if (!m_cursorDisabled)
                {
                    DisableCursor();
                    m_cursorDisabled = true;
                }
            }

            // Only update game logic if game is initialized (map selected)
            if (!consoleOpen)
            {
                UpdatePlayerLogic();
            }
            else
            {
                // Only show player metrics if game is initialized (map selected)
                auto player = GetEngine()->GetPlayer();
                auto *player = playerService ? player : nullptr;

                if (player)
                {
                    player->GetCameraController()->UpdateMouseRotation(
                        player->GetCameraController()->GetCamera(),
                        player->GetMovement()->GetPosition());
                    player->GetCameraController()->Update();

                    GetEngine()->GetRenderManager()->ShowMetersPlayer(*player->GetRenderable());
                }
            }
        }
        else
        {
            // Menu is closed but game not initialized - show cursor (shouldn't happen, but safe)
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
    auto *engine = GetEngine();
    if (!engine)
        return;

    // Get Menu via UIController
    auto moduleManager = engine->GetModuleManager();
    UIController *uiController = nullptr;
    Menu *menu = nullptr;

    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            uiController = dynamic_cast<UIController *>(uiModule);
            if (uiController)
            {
                menu = uiController->GetMenu();
            }
        }
    }

    auto player = GetEngine()->GetPlayer();
    auto *player = playerService ? player : nullptr;

    if (m_showMenu && menu)
    {
        // Need to call rlImGuiBegin before rendering menu
        rlImGuiBegin();
        // engine->GetRenderManager()->RenderMenu(*menu); // DEPRECATED - Menu renders itself via
        // ImGui

        // Render console in menu if open
        if (menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
        {
            menu->GetConsoleManager()->RenderConsole();
        }

        rlImGuiEnd();
    }
    else
    {
        // Only render game world and UI if game is initialized (map selected)
        if (m_isGameInitialized)
        {
            // Get RenderingSystem through ModuleManager
            if (auto moduleManager = engine->GetModuleManager())
            {
                auto *renderingModule = moduleManager->GetModule("Rendering");
                if (renderingModule)
                {
                    auto *renderSystem = dynamic_cast<RenderingSystem *>(renderingModule);
                    if (renderSystem)
                    {
                        renderSystem->RenderGameWorld();
                        renderSystem->RenderGameUI();
                    }
                }
            }
        }
    }

    if (engine->IsDebugInfoVisible() && !m_showMenu && player)
    {
        engine->GetRenderManager()->RenderDebugInfo(*player->GetRenderable(), *m_models,
                                                    *m_collisionManager);
    }

    // Render console in game if open (works for both menu and game)
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu) // Only call rlImGuiBegin/End if not already in menu rendering
        {
            rlImGuiBegin();
            menu->GetConsoleManager()->RenderConsole();
            rlImGuiEnd();
        }
        // If in menu, console is already rendered above
    }
}

void GameApplication::OnShutdown()
{
    TraceLog(LOG_INFO, "[GameApplication] Cleaning up game resources...");

    if (m_collisionManager && !m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        TraceLog(LOG_INFO, "[GameApplication] Collision system cleared");
    }

    // Get components through Engine
    auto player = GetEngine()->GetPlayer();
    auto *player = playerService ? player : nullptr;

    auto mapSystem = GetEngine()->GetMapSystem();
    auto *mapSystem = mapSystemService ? mapSystem : nullptr;

    // MenuService removed, access via UIController if needed, but here we just reset state

    if (player)
    {
        player->SetPlayerPosition({0.0f, 0.0f, 0.0f});
        player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
    }

    if (mapSystem && !mapSystem->GetGameMap().GetMapObjects().empty())
    {
        mapSystem->GetGameMap().Cleanup();
        TraceLog(LOG_INFO, "[GameApplication] Editor map cleared");
    }

    m_showMenu = true;
    m_isGameInitialized = false;

    // Access menu to reset state
    auto engine = GetEngine();
    if (engine)
    {
        auto moduleManager = engine->GetModuleManager();
        if (moduleManager)
        {
            auto *uiModule = moduleManager->GetModule("UI");
            if (uiModule)
            {
                auto *uiController = dynamic_cast<UIController *>(uiModule);
                if (uiController && uiController->GetMenu())
                {
                    uiController->GetMenu()->SetGameInProgress(false);
                }
            }
        }
    }

    TraceLog(LOG_INFO, "[GameApplication] Game resources cleaned up successfully");
}

void GameApplication::InitInput()
{
    TraceLog(LOG_INFO, "[GameApplication] Setting up game-specific input bindings...");

    auto *engine = GetEngine();
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
            auto *uiController = dynamic_cast<UIController *>(uiModule);
            if (uiController)
            {
                menu = uiController->GetMenu();
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
    // Get UIController through ModuleManager
    auto *engine = GetEngine();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    if (!moduleManager)
        return;

    auto *uiModule = moduleManager->GetModule("UI");
    if (!uiModule)
        return;

    auto *uiController = dynamic_cast<UIController *>(uiModule);
    if (uiController)
    {
        uiController->HandleMenuActions(&m_showMenu, &m_isGameInitialized);
    }
}

void GameApplication::UpdatePlayerLogic()
{
    // Get PlayerSystem through Engine
    auto playerSystemService = GetEngine()->GetService<PlayerSystemService>();
    if (playerSystemService && playerSystemService->playerSystem)
    {
        playerSystemService->playerSystem->UpdatePlayerLogic();
    }
}

void GameApplication::SaveGameState()
{
    if (!m_isGameInitialized)
    {
        return; // No state to save if game is not initialized
    }

    // Get PlayerSystem through Engine
    auto playerSystemService = GetEngine()->GetService<PlayerSystemService>();
    if (!playerSystemService || !playerSystemService->playerSystem)
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - PlayerSystem not available");
        return;
    }

    // Get MapSystem through Engine to get current map path
    auto mapSystem = GetEngine()->GetMapSystem();
    if (!mapSystemService || !mapSystem)
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - MapSystem not available");
        return;
    }

    // Get current map path
    std::string currentMapPath = mapSystem->GetCurrentMapPath();

    if (currentMapPath.empty())
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - Current map path is empty");
        return;
    }

    playerSystemService->playerSystem->SavePlayerState(currentMapPath);
    TraceLog(LOG_INFO, "[GameApplication] Game state saved (map: %s)", currentMapPath.c_str());
}
