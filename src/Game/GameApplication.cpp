#include "GameApplication.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Module/ModuleManager.h"
#include "Engine/Render/RenderManager.h"
#include "Engine/Config/ConfigManager.h"
#include "Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Menu/Menu.h"
#include "Managers/MapManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/StateManager.h"
#include "Managers/GameRenderHelpers.h"
#include "Managers/PlayerManager.h"
#include "Managers/UpdateManager.h"
#include "Managers/MenuActionHandler.h"
#include "Systems/PlayerSystem/PlayerSystem.h"
#include "Systems/MapSystem/MapSystem.h"
#include "Systems/UIController/UIController.h"
#include "Systems/RenderingSystem/RenderingSystem.h"
#include "Menu/ConsoleManagerHelpers.h"
#include "imgui.h"
#include "rlImGui.h"
#include <raylib.h>
#include <GLFW/glfw3.h>

GameApplication::GameApplication(int argc, char* argv[])
    : EngineApplication()
    , m_showMenu(true)
    , m_isGameInitialized(false)
    , m_cursorDisabled(false)
{
    ProcessCommandLine(argc, argv);
}

GameApplication::~GameApplication()
{
    TraceLog(LOG_INFO, "GameApplication destructor called.");
}

void GameApplication::ProcessCommandLine(int argc, char* argv[])
{
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);
    
    // Load config from game.cfg BEFORE setting window size
    ConfigManager configManager;
    bool configLoaded = false;
    
    // Try loading from bin/game.cfg (where game is launched)
    if (configManager.LoadFromFile("bin/game.cfg")) {
        TraceLog(LOG_INFO, "[GameApplication] Loaded config from bin/game.cfg");
        configLoaded = true;
    }
    // Or from current directory
    else if (configManager.LoadFromFile("game.cfg")) {
        TraceLog(LOG_INFO, "[GameApplication] Loaded config from game.cfg");
        configLoaded = true;
    }
    else {
        TraceLog(LOG_WARNING, "[GameApplication] Could not load game.cfg, using defaults");
    }
    
    // Get resolution from config (if not specified in command line)
    int width = m_gameConfig.width;
    int height = m_gameConfig.height;
    
    // If resolution not specified in command line (using defaults)
    // and config loaded, use values from config
    if ((width == 1280 && height == 720) && configLoaded) {
        configManager.GetResolution(width, height);
        TraceLog(LOG_INFO, "[GameApplication] Using resolution from config: %dx%d", width, height);
    }
    
    // Also check fullscreen from config
    if (configLoaded && !m_gameConfig.fullscreen) {
        m_gameConfig.fullscreen = configManager.IsFullscreen();
    }
    
    if (m_gameConfig.developer)
    {
        CommandLineHandler::ShowConfig(m_gameConfig);
    }
    
    // Set window configuration
    auto& config = GetConfig();
    config.width = width;
    config.height = height;
    config.windowName = "Chained Decos";
    
    TraceLog(LOG_INFO, "[GameApplication] Window config: %dx%d (fullscreen: %s)", 
             config.width, config.height, m_gameConfig.fullscreen ? "yes" : "no");
}

void GameApplication::OnPreInitialize()
{
    SetTraceLogLevel(LOG_INFO);
    TraceLog(LOG_INFO, "[GameApplication] Pre-initialization...");
}

void GameApplication::OnInitializeServices()
{
    TraceLog(LOG_INFO, "[GameApplication] Initializing engine services...");

    // Create only basic engine components needed BEFORE system initialization
    // Player, Menu and managers are now created in systems
    m_collisionManager = std::make_unique<CollisionManager>();
    m_models = std::make_unique<ModelLoader>();
    m_world = std::make_unique<WorldManager>();
    
    // Initialize managers that don't have their own systems yet
    InitializeManagers();
    
    TraceLog(LOG_INFO, "[GameApplication] Engine services initialized.");
    TraceLog(LOG_INFO, "[GameApplication] Game-specific components will be created by systems.");
}

void GameApplication::OnRegisterProjectModules()
{
    TraceLog(LOG_INFO, "[GameApplication] Registering game systems...");
    
    if (auto engine = GetEngine()) {
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
    } else {
        TraceLog(LOG_WARNING, "[GameApplication] No engine available, cannot register systems");
    }
}

void GameApplication::OnRegisterProjectServices()
{
    // First register core services
    RegisterCoreKernelServices();
    
    // Then managers
    RegisterManagerKernelServices();
}

void GameApplication::OnPostInitialize()
{
    // Initial state - show menu
    m_showMenu = true;
    
    // Initialize cursor state - menu is shown, so cursor should be enabled
    m_cursorDisabled = false;
    
    // Ensure cursor is visible when menu is shown (using system cursor)
    EnableCursor();
    
    // Configure ImGui for proper mouse/cursor handling in menu
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation
    io.MouseDrawCursor = false; // Use system cursor, not ImGui cursor
    
    // Systems now initialized, get components through Kernel
    auto playerService = GetKernel()->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto menuService = GetKernel()->GetService<MenuService>(Kernel::ServiceType::Menu);
    auto mapService = GetKernel()->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    auto playerManagerService = GetKernel()->GetService<PlayerManagerService>(Kernel::ServiceType::PlayerManager);
    
    auto* player = playerService ? playerService->player : nullptr;
    auto* menu = menuService ? menuService->menu : nullptr;
    auto* mapManager = mapService ? mapService->mapManager : nullptr;
    auto* playerManager = playerManagerService ? playerManagerService->playerManager : nullptr;
    
    // Dependency Injection: update ConsoleManager providers after all services are registered
    UpdateConsoleManagerProviders(GetKernel());
    
    if (player && menu) {
        m_renderHelper = std::make_unique<GameRenderHelpers>(m_collisionManager.get());
    }
    if (m_collisionManager && mapManager) {
        m_updateManager = std::make_unique<UpdateManager>(m_collisionManager.get(), mapManager);
    }
    // Create managers that need components from systems
    if (player && menu) {
        m_stateManager = std::make_unique<StateManager>(player, menu);
    }
    

    // Raw mouse motion is now initialized in Engine::Init() to avoid duplication
    // GameRenderManager replaced with RenderingSystem
    // RenderingSystem is created and initialized through ModuleManager
    
    
    if (menu && player) {
        m_menuActionHandler = std::make_unique<MenuActionHandler>(
            GetKernel(), &m_showMenu, &m_isGameInitialized);
    }
    
    // Register StateManagerService after creation
    if (m_stateManager) {
        auto kernel = GetKernel();
        if (kernel) {
            kernel->RegisterService<StateManagerService>(Kernel::ServiceType::StateManager,
                std::make_shared<StateManagerService>(m_stateManager.get()));
            TraceLog(LOG_INFO, "[GameApplication] StateManagerService registered.");
        }
    }
    
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
    if (m_gameConfig.fullscreen && !IsWindowFullscreen()) {
        TraceLog(LOG_INFO, "[GameApplication] Setting fullscreen mode from config");
        // Use raylib function to toggle fullscreen
        // (ToggleFullscreen registered as callback for F11, but we can call directly via SetWindowState)
        int monitor = GetCurrentMonitor();
        int monitorWidth = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);
        SetWindowSize(monitorWidth, monitorHeight);
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }
    
    TraceLog(LOG_INFO, "[GameApplication] Game application initialized (player will be initialized when map is selected).");
}

void GameApplication::OnPostUpdate(float deltaTime)
{
    (void)deltaTime;  // Unused for now
    
    // Get Menu through Kernel
    auto menuService = GetKernel()->GetService<MenuService>(Kernel::ServiceType::Menu);
    auto* menu = menuService ? menuService->menu : nullptr;
    
    if (IsKeyPressed(KEY_GRAVE) && menu)
    {
        menu->ToggleConsole();
    }

    // Manage cursor visibility based on menu state
    // Only call EnableCursor/DisableCursor when state changes to avoid centering cursor every frame
    if (m_showMenu)
    {
        // Menu is open - show system cursor (more reliable than ImGui cursor)
        ImGuiIO& io = ImGui::GetIO();
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
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        
        // Force clear ImGui navigation state when menu closes to prevent input blocking
        // This ensures player input works immediately after closing menu
        if (io.NavActive)
        {
            io.NavActive = false;
            io.NavVisible = false;
        }
        
        // Game is running - check console state
        if (m_isGameInitialized)
        {
            // Game is running - check console state
            bool consoleOpen = menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen();
            
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
                UpdatePhysicsLogic();
            }
            else
            {
                // Only show player metrics if game is initialized (map selected)
                auto playerService = GetKernel()->GetService<PlayerService>(Kernel::ServiceType::Player);
                auto* player = playerService ? playerService->player : nullptr;
                
                if (player)
                {
                    
                    //player->GetCameraController()->UpdateCameraRotation();
                    player->GetCameraController()->UpdateMouseRotation(
                    player->GetCameraController()->GetCamera(), player->GetMovement()->GetPosition());
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

void GameApplication::OnPostRender()
{
    auto* engine = GetEngine();
    if (!engine) return;

    // Get Menu and Player through Kernel
    auto menuService = GetKernel()->GetService<MenuService>(Kernel::ServiceType::Menu);
    auto* menu = menuService ? menuService->menu : nullptr;
    
    auto playerService = GetKernel()->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto* player = playerService ? playerService->player : nullptr;
    
    if (m_showMenu && menu)
    {
        // Need to call rlImGuiBegin before rendering menu
        rlImGuiBegin();
        engine->GetRenderManager()->RenderMenu(*menu);
        
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
            if (auto moduleManager = engine->GetModuleManager()) {
                auto* renderingModule = moduleManager->GetModule("Rendering");
                if (renderingModule) {
                    auto* renderSystem = dynamic_cast<RenderingSystem*>(renderingModule);
                    if (renderSystem) {
                        renderSystem->RenderGameWorld();
                        renderSystem->RenderGameUI();
                    }
                }
            }
        }
    }

    if (engine->IsDebugInfoVisible() && !m_showMenu && player)
    {
        engine->GetRenderManager()->RenderDebugInfo(*player->GetRenderable(), *m_models, *m_collisionManager);
    }

    // Render console in game if open (works for both menu and game)
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu)  // Only call rlImGuiBegin/End if not already in menu rendering
        {
            rlImGuiBegin();
            menu->GetConsoleManager()->RenderConsole();
            rlImGuiEnd();
        }
        // If in menu, console is already rendered above
    }
}

void GameApplication::OnPreShutdown()
{
    TraceLog(LOG_INFO, "[GameApplication] Cleaning up game resources...");

    if (m_collisionManager && !m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        TraceLog(LOG_INFO, "[GameApplication] Collision system cleared");
    }

    // Get components through Kernel (they're deleted by systems)
    auto playerService = GetKernel()->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto* player = playerService ? playerService->player : nullptr;
    
    auto mapService = GetKernel()->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    auto* mapManager = mapService ? mapService->mapManager : nullptr;
    
    auto menuService = GetKernel()->GetService<MenuService>(Kernel::ServiceType::Menu);
    auto* menu = menuService ? menuService->menu : nullptr;

    if (player)
    {
        player->SetPlayerPosition({0.0f, 0.0f, 0.0f});
        player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
    }

    if (mapManager && !mapManager->GetGameMap().GetMapObjects().empty())
    {
        mapManager->GetGameMap().Cleanup();
        TraceLog(LOG_INFO, "[GameApplication] Editor map cleared");
    }

    m_showMenu = true;
    m_isGameInitialized = false;
    if (menu)
    {
        menu->SetGameInProgress(false);
    }

    TraceLog(LOG_INFO, "[GameApplication] Game resources cleaned up successfully");
}

void GameApplication::RegisterCoreKernelServices()
{
    TraceLog(LOG_INFO, "[GameApplication] Registering core engine services...");
    
    auto kernel = GetKernel();
    if (!kernel) return;
    
    // Register only basic engine services
    // Player and Menu are registered by their systems (PlayerSystem, UIController)
    kernel->RegisterService<CollisionService>(Kernel::ServiceType::Collision,
                                             std::make_shared<CollisionService>(m_collisionManager.get()));
    
    kernel->RegisterService<ModelsService>(Kernel::ServiceType::Models,
                                         std::make_shared<ModelsService>(m_models.get()));
    
    kernel->RegisterService<WorldService>(Kernel::ServiceType::World,
                                        std::make_shared<WorldService>(m_world.get()));
    
    TraceLog(LOG_INFO, "[GameApplication] Core engine services registered.");
    TraceLog(LOG_INFO, "[GameApplication] Game services will be registered by systems.");
}

void GameApplication::RegisterManagerKernelServices()
{
    TraceLog(LOG_INFO, "[GameApplication] Registering manager services...");
    
    auto kernel = GetKernel();
    if (!kernel) return;
    
    // MapManager and PlayerManager are registered by their systems (MapSystem, PlayerSystem)
    // Only register ResourceManager which doesn't have its own system yet
    if (m_modelManager) {
        kernel->RegisterService<ResourceManagerService>(Kernel::ServiceType::ResourceManager,
                                                       std::make_shared<ResourceManagerService>(m_modelManager.get()));
    }
    
    TraceLog(LOG_INFO, "[GameApplication] Manager services registered.");
    TraceLog(LOG_INFO, "[GameApplication] MapManager and PlayerManager registered by their systems.");
}

void GameApplication::InitializeManagers()
{
    TraceLog(LOG_INFO, "[GameApplication] Creating remaining manager components...");

    // MapManager and PlayerManager are now created in systems
    // Only create managers that don't have their own systems yet
    m_modelManager = std::make_unique<ResourceManager>(m_models.get());
    
    // StateManager, MenuActionHandler, UpdateManager remain here for now
    // GameRenderManager already converted to RenderingSystem
    // They will be initialized later through OnPostInitialize after systems create their components
    // TODO: Convert to systems
    
    TraceLog(LOG_INFO, "[GameApplication] Manager components initialized.");
    TraceLog(LOG_INFO, "[GameApplication] MapManager and PlayerManager created by their systems.");
}

void GameApplication::InitInput()
{
    TraceLog(LOG_INFO, "[GameApplication] Setting up game-specific input bindings...");

    auto* engine = GetEngine();
    if (!engine)
    {
        TraceLog(LOG_WARNING, "[GameApplication] No engine provided, skipping input bindings");
        return;
    }

    // Get Menu through Kernel
    auto menuService = GetKernel()->GetService<MenuService>(Kernel::ServiceType::Menu);
    auto* menu = menuService ? menuService->menu : nullptr;
    
    if (!menu) {
        TraceLog(LOG_WARNING, "[GameApplication] Menu not found, skipping input bindings");
        return;
    }

        engine->GetInputManager().RegisterAction(KEY_F1,
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

    engine->GetInputManager().RegisterAction(KEY_ESCAPE,
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
    if (m_menuActionHandler)
    {
        m_menuActionHandler->HandleMenuActions();
    }
}

void GameApplication::UpdatePlayerLogic()
{
    // Get PlayerManager through Kernel
    auto playerManagerService = GetKernel()->GetService<PlayerManagerService>(Kernel::ServiceType::PlayerManager);
    if (playerManagerService && playerManagerService->playerManager)
    {
        playerManagerService->playerManager->UpdatePlayerLogic();
    }
}

void GameApplication::UpdatePhysicsLogic()
{
    if (m_updateManager)
    {
        m_updateManager->UpdatePhysicsLogic();
    }
}

void GameApplication::SaveGameState()
{
    if (!m_stateManager || !m_isGameInitialized)
    {
        return; // No state to save if game is not initialized
    }
    
    // Get MapManager through Kernel to get current map path
    auto mapService = GetKernel()->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    if (!mapService || !mapService->mapManager)
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - MapManager not available");
        return;
    }
    
    // Get current map path (need to make GetCurrentMapPath public or add public method)
    // For now, we'll use a workaround - get from MapManager if it has public access
    std::string currentMapPath = mapService->mapManager->GetCurrentMapPath();
    
    if (currentMapPath.empty())
    {
        TraceLog(LOG_WARNING, "[GameApplication] SaveGameState() - Current map path is empty");
        return;
    }
    
    m_stateManager->SaveGameState(currentMapPath);
    TraceLog(LOG_INFO, "[GameApplication] Game state saved (map: %s)", currentMapPath.c_str());
}

// RenderGameWorld() and RenderGameUI() are now in RenderingSystem
// Called through OnPostRender() -> RenderingSystem::RenderGameWorld/UI()

