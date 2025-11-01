#include "Game.h"
#include "Managers/MapManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/StateManager.h"
#include "Managers/RenderHelper.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"
#include "Engine/Engine.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Map/MapLoader.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Game/Menu/Menu.h"
#include "Player/PlayerCollision.h"
#include "imgui.h"
#include "rlImGui.h"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>


Game::Game() : m_showMenu(true), m_isGameInitialized(false), m_isDebugInfo(true)
{
    TraceLog(LOG_INFO, "Game class instance created.");
}

void Game::Cleanup()
{
    TraceLog(LOG_INFO, "Game::Cleanup() - Cleaning up game resources...");

    // Clear collision system
    if (!m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        TraceLog(LOG_INFO, "Game::Cleanup() - Collision system cleared");
    }

    // Reset player state
    m_player->SetPlayerPosition({0.0f, 0.0f, 0.0f});
    m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Clear any loaded maps
    if (!m_mapManager->GetGameMap().objects.empty())
    {
        m_mapManager->GetGameMap().Cleanup();
        TraceLog(LOG_INFO, "Game::Cleanup() - Editor map cleared");
    }

    // Reset game state
    m_showMenu = true;
    m_isGameInitialized = false;
    m_menu->SetGameInProgress(false); // Clear game state when cleaning up

    TraceLog(LOG_INFO, "Game::Cleanup() - Game resources cleaned up successfully");
}

Game::~Game()
{
    TraceLog(LOG_INFO, "Game class destructor called.");
    // Note: Cleanup() should be called explicitly before destruction
    // as destructors should not throw exceptions
}

void Game::Init(int argc, char *argv[])
{
    TraceLog(LOG_INFO, "Game::Init() - Initializing game components...");

    // Parse command line arguments
    GameConfig config = CommandLineHandler::ParseArguments(argc, argv);

    // Show parsed configuration in developer mode
    if (config.developer)
    {
        CommandLineHandler::ShowConfig(config);
    }

    // Create core components
    auto renderManager = std::make_shared<RenderManager>();
    auto inputManager = std::make_shared<InputManager>();

    // Create and initialize kernel first
    m_kernel = std::make_unique<Kernel>();
    m_kernel->Initialize();

    // Initialize engine with kernel reference
    m_engine = std::make_unique<Engine>(config.width, config.height, renderManager, inputManager,
                                        m_kernel.get());
    m_engine->Init();

    // Initialize game components
    m_player = std::make_unique<Player>();
    m_collisionManager = std::make_unique<CollisionManager>();
    m_models = std::make_unique<ModelLoader>();
    m_world = std::make_unique<WorldManager>();
    m_menu = std::make_unique<Menu>();

    // Set log level to show INFO messages for debugging
    SetTraceLogLevel(LOG_INFO);

    TraceLog(LOG_INFO, "Game::Init() - About to initialize menu...");
    // Initialize menu with engine reference
    m_menu->Initialize(m_engine.get());
    // Set game reference for console manager
    m_menu->SetGame(this);
    TraceLog(LOG_INFO, "Game::Init() - Menu initialized.");

    // Initialize manager components
    m_mapManager = std::make_unique<MapManager>(m_player.get(), m_collisionManager.get(), 
                                                     m_models.get(), m_engine->GetRenderManager(),
                                                     m_kernel.get(), m_menu.get());
    m_modelManager = std::make_unique<ResourceManager>(m_models.get());
    m_stateManager = std::make_unique<StateManager>(m_player.get(), m_menu.get());
    m_renderHelper = std::make_unique<RenderHelper>(m_collisionManager.get());
    TraceLog(LOG_INFO, "Game::Init() - Manager components initialized.");

    TraceLog(LOG_INFO, "Game::Init() - Kernel already initialized.");

    // Only register engine-dependent services if engine is available
    if (m_engine)
    {
        // InputManager is already registered by Engine
        TraceLog(LOG_INFO, "Game::Init() - InputManager already registered by Engine");
    }
    else
    {
        TraceLog(LOG_WARNING,
                 "Game::Init() - No engine provided, skipping engine-dependent services");
    }

    m_kernel->RegisterService<ModelsService>(Kernel::ServiceType::Models,
                                             std::make_shared<ModelsService>(m_models.get()));
    m_kernel->RegisterService<WorldService>(Kernel::ServiceType::World,
                                            std::make_shared<WorldService>(m_world.get()));
    InitPlayer();
    InitInput();

    m_isGameInitialized = true;
    TraceLog(LOG_INFO, "Game::Init() - Game components initialized.");

    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    SetWindowIcon(m_icon);
    UnloadImage(m_icon);
}

void Game::Run()
{
    TraceLog(LOG_INFO, "Game::Run() - Starting game loop...");

    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "Game::Run() - No engine provided, cannot run game loop");
        return;
    }

    while (!m_engine->ShouldClose())
    {
        Update();
        Render();
    }

    TraceLog(LOG_INFO, "Game::Run() - Game loop ended.");
}

void Game::Update()
{
    // Update engine (handles window and timing) - only if engine is available
    if (m_engine)
    {
        m_engine->Update();
        m_engine->GetInputManager().ProcessInput();
    }

    // Update kernel services each frame
    m_kernel->Update(GetFrameTime());

    // Handle console input (works in both menu and gameplay)
    if (IsKeyPressed(KEY_GRAVE)) // ~ key
    {
        m_menu->ToggleConsole();
    }

    // Console input is handled internally by the menu

    // Input processing is handled by Kernel services

    if (m_showMenu)
    {
        HandleMenuActions();
    }
    else
    {
        // Check if console is open - if so, pause player updates to prevent falling
        bool consoleOpen = m_menu->GetConsoleManager() && m_menu->GetConsoleManager()->IsConsoleOpen();
        if (!consoleOpen)
        {
            // Update game logic only when console is closed
            // Update player logic
            UpdatePlayerLogic();
            UpdatePhysicsLogic();
        }
        else
        {
            // Console is open - only update camera and UI, but freeze player physics
            const ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse)
            {
                // Still update camera rotation even when console is open
                m_player->GetCameraController()->UpdateCameraRotation();
                m_player->GetCameraController()->UpdateMouseRotation(
                    m_player->GetCameraController()->GetCamera(), m_player->GetMovement()->GetPosition());
                m_player->GetCameraController()->Update();
            }
            m_engine->GetRenderManager()->ShowMetersPlayer(*m_player);
        }
    }
}

void Game::Render()
{
    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "Game::Render() - No engine provided, skipping render");
        return;
    }

    m_engine->GetRenderManager()->BeginFrame();

    if (m_showMenu)
    {
        m_engine->GetRenderManager()->RenderMenu(*m_menu);
    }
    else
    {
        // For debugging, only log model stats when they change
        static int lastInstanceCount = 0;
        int currentInstanceCount = m_models->GetLoadingStats().totalInstances;
        if (currentInstanceCount != lastInstanceCount)
        {
            lastInstanceCount = currentInstanceCount;
        }

        RenderGameWorld();
        RenderGameUI();

    }

    if (m_engine->IsDebugInfoVisible() && !m_showMenu)
    {
        m_engine->GetRenderManager()->RenderDebugInfo(*m_player, *m_models, *m_collisionManager);
    }

    // Render console if open and not in menu
    if (!m_showMenu && m_menu->GetConsoleManager() && m_menu->GetConsoleManager()->IsConsoleOpen())
    {
        rlImGuiBegin();
        m_menu->GetConsoleManager()->RenderConsole();
        rlImGuiEnd();
    }

    m_engine->GetRenderManager()->EndFrame();
    // Optional kernel render pass hook
    m_kernel->Render();
}

void Game::ToggleMenu()
{
    m_showMenu = !m_showMenu;
    if (m_showMenu)
    {
        EnableCursor();
    }
    else
    {
        HideCursor();
    }
    TraceLog(LOG_INFO, "Menu toggled: %s", m_showMenu ? "ON" : "OFF");
}

// Helper functions for cursor management
void Game::EnableCursor()
{
    if (m_engine)
    {
        // Enable cursor through engine if available
        TraceLog(LOG_INFO, "Game::EnableCursor() - Cursor enabled");
    }
    else
    {
        TraceLog(LOG_INFO, "Game::EnableCursor() - No engine, cursor state unchanged");
    }
}

void Game::HideCursor()
{
    if (m_engine)
    {
        // Hide cursor through engine if available
        TraceLog(LOG_INFO, "Game::HideCursor() - Cursor hidden");
    }
    else
    {
        TraceLog(LOG_INFO, "Game::HideCursor() - No engine, cursor state unchanged");
    }
}

void Game::RequestExit() const
{
    if (m_engine)
    {
        m_engine->RequestExit();
    }
    TraceLog(LOG_INFO, "Game exit requested.");
}

bool Game::IsRunning() const
{
    if (m_engine)
    {
        return !m_engine->ShouldClose();
    }
    return false; // If no engine, consider not running
}

void Game::InitInput()
{
    TraceLog(LOG_INFO, "Game::InitInput() - Setting up game-specific input bindings...");

    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "Game::InitInput() - No engine provided, skipping input bindings");
        return;
    }

    m_engine->GetInputManager().RegisterAction(KEY_F1,
                                               [this]
                                               {
                                                   // Set game as in progress when going to menu
                                                   // from game
                                                   if (!m_showMenu)
                                                   {
                                                       m_menu->SetGameInProgress(true);
                                                   }
                                                   m_showMenu = true;
                                                   EnableCursor();
                                               });

    m_engine->GetInputManager().RegisterAction(KEY_ESCAPE,
                                               [this]
                                               {
                                                   if (!m_showMenu)
                                                   {
                                                       // Save current game state before pausing
                                                       SaveGameState();

                                                       m_menu->ResetAction();
                                                       // Set game as in progress when going to menu
                                                       // from game
                                                       m_menu->SetGameInProgress(true);
                                                       ToggleMenu();
                                                       EnableCursor();
                                                   }
                                               });
    TraceLog(LOG_INFO, "Game::InitInput() - Game input bindings configured.");
}

void Game::InitCollisions()
{
    m_mapManager->InitCollisions();
    
    // Update game initialization state based on menu action
    MenuAction action = m_menu->GetAction();
    switch (action)
    {
    case MenuAction::StartGameWithMap:
        m_isGameInitialized = true;
        break;
    default:
        m_isGameInitialized = false;
        break;
    }
}

void Game::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    m_mapManager->InitCollisionsWithModels(requiredModels);
    
    // Update game initialization state based on menu action
    MenuAction action = m_menu->GetAction();
    switch (action)
    {
    case MenuAction::StartGameWithMap:
        m_isGameInitialized = true;
        break;
    default:
        m_isGameInitialized = false;
        break;
    }
}

bool Game::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    return m_mapManager->InitCollisionsWithModelsSafe(requiredModels);
}

void Game::InitPlayer()
{

    // Set initial position on the first platform (mix of ground and floating platforms)
    Vector3 safePosition = {0.0f, PLAYER_SAFE_SPAWN_HEIGHT, 0.0f};
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting initial safe position: (%.2f, %.2f, %.2f)",
             safePosition.x, safePosition.y, safePosition.z);
    m_player->SetPlayerPosition(safePosition);

    // Setup collision and physics
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting up collision manager for player...");
    m_player->GetMovement()->SetCollisionManager(m_collisionManager.get());

    TraceLog(LOG_INFO, "Game::InitPlayer() - Updating player collision box...");
    m_player->UpdatePlayerBox();

    TraceLog(LOG_INFO, "Game::InitPlayer() - Updating player collision...");
    m_player->UpdatePlayerCollision();

    // Allow physics to determine grounded state; start ungrounded so gravity applies
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting initial physics state...");
    m_player->GetPhysics().SetGroundLevel(false);
    m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Load player model with improved error handling and fallback
    TraceLog(LOG_INFO, "Game::InitPlayer() - Loading player model...");
    // Try to load the player model
    if (auto playerModel = m_models->GetModelByName("player_low"))
    {
        Model &model = playerModel->get();
        TraceLog(LOG_INFO, "Game::InitPlayer() - Player model pointer: %p, meshCount: %d", &model,
                 model.meshCount);

        if (model.meshCount > 0)
        {
            m_player->SetPlayerModel(&model);
            TraceLog(LOG_INFO, "Game::InitPlayer() - Player model loaded successfully.");
        }
        else
        {
            TraceLog(LOG_ERROR, "Game::InitPlayer() - Player model is invalid or has no meshes");
            // Try to load player_low.glb directly if player.glb failed
            if (!m_models->LoadSingleModel("player", PROJECT_ROOT_DIR "/resources/player_low.glb", true))
            {
                TraceLog(LOG_ERROR,
                         "Game::InitPlayer() - Failed to load player_low.glb as fallback");
            }
            else
            {
                TraceLog(LOG_INFO,
                         "Game::InitPlayer() - Successfully loaded player_low.glb as fallback");
                if (auto playerModel = m_models->GetModelByName("player"))
                {
                    Model &model = playerModel->get();
                    if (model.meshCount > 0)
                    {
                        m_player->SetPlayerModel(&model);
                        TraceLog(
                            LOG_INFO,
                            "Game::InitPlayer() - Player model loaded successfully with fallback.");
                    }
                }
            }
        }
    }

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
             safePosition.x, safePosition.y, safePosition.z);

    // Additional safety check - ensure player is properly positioned
    Vector3 currentPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO, "Game::InitPlayer() - Player current position: (%.2f, %.2f, %.2f)",
             currentPos.x, currentPos.y, currentPos.z);

    // Validate player position is safe (above ground but not too high)
    if (currentPos.y < 0.0f)
    {
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player position below ground level, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    else if (currentPos.y > 50.0f)
    {
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player position too high, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }

    // Check if map has PlayerStart objects and adjust position accordingly
    TraceLog(LOG_INFO, "Game::InitPlayer() - Checking for PlayerStart objects in map...");
    if (!m_mapManager->GetGameMap().objects.empty())
    {
        TraceLog(LOG_INFO, "Game::InitPlayer() - Map has %d objects, searching for PlayerStart...",
                 m_mapManager->GetGameMap().objects.size());
        for (size_t i = 0; i < m_mapManager->GetGameMap().objects.size(); ++i)
        {
            const auto &obj = m_mapManager->GetGameMap().objects[i];
            TraceLog(LOG_INFO, "Game::InitPlayer() - Checking object %d: %s (type: %d)", i,
                     obj.name.c_str(), static_cast<int>(obj.type));

            if ((obj.type == MapObjectType::MODEL || obj.type == MapObjectType::LIGHT) &&
                obj.name.find("player_start") != std::string::npos)
            {
                TraceLog(LOG_INFO,
                         "Game::InitPlayer() - Found PlayerStart object at (%.2f, %.2f, %.2f)",
                         obj.position.x, obj.position.y, obj.position.z);
                m_player->SetPlayerPosition(obj.position);
                TraceLog(LOG_INFO,
                         "Game::InitPlayer() - Player position updated to PlayerStart location");
                break;
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "Game::InitPlayer() - No map objects found, using default position");
    }

    // Final position verification
    Vector3 finalPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO, "Game::InitPlayer() - Final player position: (%.2f, %.2f, %.2f)", finalPos.x,
             finalPos.y, finalPos.z);

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialization complete");
}

std::optional<ModelLoader::LoadResult> Game::LoadGameModels()
{
    return m_modelManager->LoadGameModels();
}

std::optional<ModelLoader::LoadResult>
Game::LoadGameModelsSelective(const std::vector<std::string> &modelNames)
{
    return m_modelManager->LoadGameModelsSelective(modelNames);
}

std::optional<ModelLoader::LoadResult>
Game::LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames)
{
    return m_modelManager->LoadGameModelsSelectiveSafe(modelNames);
}

std::string Game::GetModelNameForObjectType(int objectType, const std::string &modelName)
{
    return m_modelManager->GetModelNameForObjectType(objectType, modelName);
}

std::vector<std::string> Game::GetModelsRequiredForMap(const std::string &mapIdentifier)
{
    return m_modelManager->GetModelsRequiredForMap(mapIdentifier);
}

void Game::UpdatePlayerLogic()
{
    if (!m_engine)
    {
        // Skip player logic if no engine is available (for testing)
        m_player->Update(*m_collisionManager);
        return;
    }

    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        // Still update camera rotation even when ImGui wants mouse capture
        // This allows camera to work when menu is open or when hovering over UI
        m_player->GetCameraController()->UpdateCameraRotation();
        m_player->GetCameraController()->UpdateMouseRotation(
            m_player->GetCameraController()->GetCamera(), m_player->GetMovement()->GetPosition());
        m_player->GetCameraController()->Update();

        m_engine->GetRenderManager()->ShowMetersPlayer(*m_player);
        // return;
    }

    m_player->Update(*m_collisionManager);

    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player);
}

void Game::UpdatePhysicsLogic()
{
    const auto &colliders = m_collisionManager->GetColliders();

    if (colliders.empty())
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders available for physics in "
                                "Game::UpdatePhysicsLogic()!");
            warningShown = true;
        }

        // Create emergency ground plane if no colliders exist and no custom map is loaded
        if (m_mapManager->GetGameMap().objects.empty())
        {
            TraceLog(LOG_WARNING,
                     "Game::UpdatePhysicsLogic() - No colliders and no map objects loaded");
        }
        else
        {
            TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - No colliders but custom map "
                                  "loaded, using map objects for collision.");
        }
    }
    else if (colliders.size() < 2) // Only ground plane exists
    {
        static bool infoShown = false;
        if (!infoShown)
        {
            TraceLog(
                LOG_INFO,
                "Game::UpdatePhysicsLogic() - Only ground plane available, no gameplay platforms");
            infoShown = true;
        }
    }
}

void Game::HandleMenuActions()
{
    MenuAction action = m_menu->ConsumeAction(); // Use ConsumeAction instead of GetAction
    switch (action)
    {
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting singleplayer...");
        m_menu->SetGameInProgress(true);

        // Initialize player after map is loaded
        try
        {
            InitPlayer();
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized successfully");
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize player: %s",
                     e.what());
            TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Player may not render correctly");
        }

        ToggleMenu();
        m_isGameInitialized = true; // Mark game as initialized
        break;

    case MenuAction::ResumeGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resuming game...");
        m_menu->SetAction(MenuAction::SinglePlayer);
        // Ensure game is properly initialized for resume
        if (!m_isGameInitialized)
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing game for resume...");

            // Load models for the current map (use saved map)
            std::vector<std::string> requiredModels = GetModelsRequiredForMap(m_mapManager->GetCurrentMapPath());
            LoadGameModelsSelective(requiredModels);

            // Initialize basic collision system first
            if (!InitCollisionsWithModelsSafe(requiredModels))
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize basic "
                                    "collision system for singleplayer");
                TraceLog(LOG_ERROR,
                         "Game::HandleMenuActions() - Cannot continue without collision system");
                return;
            }
            TraceLog(LOG_INFO,
                     "Game::HandleMenuActions() - Collision system initialized for singleplayer");

            // Initialize player after map is loaded
            try
            {
                InitPlayer();
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized for resume");
            }
            catch (const std::exception &e)
            {
                TraceLog(LOG_ERROR,
                         "Game::HandleMenuActions() - Failed to initialize player for resume: %s",
                         e.what());
                TraceLog(LOG_WARNING,
                         "Game::HandleMenuActions() - Player may not render correctly");
            }
        }
        else
        {
            // Game is already initialized, just ensure collision system is ready
            if (m_collisionManager->GetColliders().empty())
            {
                TraceLog(LOG_WARNING,
                         "Game::HandleMenuActions() - No colliders found, reinitializing...");
                // Recalculate required models for the saved map
                std::vector<std::string> requiredModels = GetModelsRequiredForMap(m_mapManager->GetCurrentMapPath());

                // Reinitialize collision system safely
                try
                {
                    // Clear existing colliders
                    m_collisionManager->ClearColliders();

                    // Ground is now provided by map objects, no artificial ground needed

                    // Initialize collision manager
                    m_collisionManager->Initialize();

                    // Try to create model collisions, but don't fail if it doesn't work
                    try
                    {
                        m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models,
                                                                                    requiredModels);
                        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resume model collisions "
                                           "created successfully");
                    }
                    catch (const std::exception &modelCollisionException)
                    {
                        TraceLog(LOG_WARNING,
                                 "Game::HandleMenuActions() - Resume model collision creation "
                                 "failed: %s",
                                 modelCollisionException.what());
                        TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Continuing with basic "
                                              "collision system only");
                    }
                }
                catch (const std::exception &e)
                {
                    TraceLog(LOG_ERROR,
                             "Game::HandleMenuActions() - Failed to reinitialize collision system "
                             "for resume: %s",
                             e.what());
                }
            }

            // Ensure player is properly positioned and set up
            if (m_player->GetPlayerPosition().x == 0.0f &&
                m_player->GetPlayerPosition().y == 0.0f && m_player->GetPlayerPosition().z == 0.0f)
            {
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player position is origin, "
                                   "resetting to safe position");
                m_player->SetPlayerPosition({0.0f, PLAYER_SAFE_SPAWN_HEIGHT, 0.0f});
            }

            // Re-setup player collision and movement
            m_player->GetMovement()->SetCollisionManager(m_collisionManager.get());
            m_player->UpdatePlayerBox();
            m_player->UpdatePlayerCollision();
        }

        // Hide the menu and resume the game
        m_showMenu = false;
        HideCursor();
        m_menu->ResetAction();
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Game resumed successfully");
        // Keep game in progress state when resuming
        break;
    case MenuAction::StartGameWithMap:
    {
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting game with selected map...");
        m_menu->SetGameInProgress(true);
        std::string selectedMapName = m_menu->GetSelectedMapName();
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Selected map: %s", selectedMapName.c_str());

        // Convert map name to full path
        std::string mapPath;
        if (selectedMapName.length() >= 3 && isalpha(selectedMapName[0]) &&
            selectedMapName[1] == ':' &&
            (selectedMapName[2] == '/' || selectedMapName[2] == '\\'))
        {
            // Already an absolute path, use as-is
            mapPath = selectedMapName;
        }
        else
        {
            // For relative paths, construct path using PROJECT_ROOT_DIR and resources/maps/
            // Extract filename to handle cases like "../maporigin.json"
            std::string filename = std::filesystem::path(selectedMapName).filename().string();
            mapPath = PROJECT_ROOT_DIR "/resources/maps/" + filename;
            if (filename.find(".json") == std::string::npos)
            {
                mapPath += ".json";
            }
        }
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Full map path: %s", mapPath.c_str());

        // Step 1: Analyze map to determine required models
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Analyzing map to determine required models...");
        std::vector<std::string> requiredModels;

        try
        {
            requiredModels = GetModelsRequiredForMap(mapPath);
            if (requiredModels.empty())
            {
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - No models required for map, but player model is always needed");
                requiredModels.emplace_back("player_low"); // Always include player model
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to analyze map for required models: %s", e.what());
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without model analysis");
            return;
        }

        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Required models for map:");
        for (const auto &model : requiredModels)
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() -   - %s", model.c_str());
        }
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Total models required: %d", requiredModels.size());

        // Step 2: Load only the required models selectively
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading required models selectively...");
        auto loadResult = LoadGameModelsSelective(requiredModels);
        if (!loadResult || loadResult->loadedModels == 0)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load any required models");
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without models");
            return;
        }
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Successfully loaded %d/%d required models in %.2f seconds",
                 loadResult->loadedModels, loadResult->totalModels, loadResult->loadingTime);

        // Step 3: Initialize collision system with required models
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing collision system with required models...");
        if (!InitCollisionsWithModelsSafe(requiredModels))
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize collision system with required models");
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without collision system");
            return;
        }
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Collision system initialized successfully");

        // Step 4: Load the map objects
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading map objects...");
        try
        {
            // Try to detect map format and use appropriate loader
            std::ifstream testFile(mapPath);
            if (testFile.is_open())
            {
                std::string firstLine;
                std::getline(testFile, firstLine);
                testFile.close();

                // Check if this looks like array format (old models.json format)
                if (firstLine.find("[") == 0)
                {
                    TraceLog(LOG_INFO, "Game::HandleMenuActions() - Detected array format, using LoadGameMap");
                    m_mapManager->GetGameMap() = LoadGameMap(mapPath.c_str());

                    // Register any models that MapLoader preloaded into the GameMap
                    if (!m_mapManager->GetGameMap().loadedModels.empty())
                    {
                        TraceLog(LOG_INFO,
                                 "Game::HandleMenuActions() - Registering %d preloaded models from map into ModelLoader",
                                 m_mapManager->GetGameMap().loadedModels.size());
                        for (const auto &p : m_mapManager->GetGameMap().loadedModels)
                        {
                            const std::string &modelName = p.first;
                            const ::Model &loaded = p.second;

                            // Validate model before registration
                            if (loaded.meshCount > 0)
                            {
                                if (m_models->RegisterLoadedModel(modelName, loaded))
                                {
                                    TraceLog(LOG_INFO,
                                             "Game::HandleMenuActions() - Successfully registered model from map: %s (meshCount: %d)",
                                             modelName.c_str(), loaded.meshCount);
                                }
                                else
                                {
                                    TraceLog(LOG_WARNING,
                                             "Game::HandleMenuActions() - Failed to register model from map: %s",
                                             modelName.c_str());
                                }
                            }
                            else
                            {
                                TraceLog(LOG_WARNING,
                                         "Game::HandleMenuActions() - Skipping invalid model from map: %s (meshCount: %d)",
                                         modelName.c_str(), loaded.meshCount);
                            }
                        }
                    }
                    else
                    {
                        TraceLog(LOG_INFO, "Game::HandleMenuActions() - No preloaded models in GameMap to register");
                    }

                    TraceLog(LOG_INFO,
                             "Game::HandleMenuActions() - Creating model instances for array-format map (%d objects)",
                             m_mapManager->GetGameMap().objects.size());
                    for (const auto &object : m_mapManager->GetGameMap().objects)
                    {
                        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
                        {
                            std::string requested = object.modelName;
                            auto available = m_models->GetAvailableModels();
                            bool exists = (std::find(available.begin(), available.end(), requested) != available.end());
                            std::string candidateName = requested;

                            if (!exists)
                            {
                                std::string stem = std::filesystem::path(requested).stem().string();
                                if (!stem.empty() && std::find(available.begin(), available.end(), stem) != available.end())
                                {
                                    candidateName = stem;
                                    exists = true;
                                }
                                else
                                {
                                    std::vector<std::string> exts = {".glb", ".gltf", ".obj"};
                                    for (const auto &ext : exts)
                                    {
                                        std::string resourcePath = std::string(PROJECT_ROOT_DIR) + "/resources/" + requested;
                                        if (std::filesystem::path(requested).extension().empty())
                                            resourcePath = std::string(PROJECT_ROOT_DIR) + "/resources/" + requested + ext;

                                        TraceLog(LOG_INFO,
                                                 "Game::HandleMenuActions() - Attempting to auto-load model '%s' from %s",
                                                 requested.c_str(), resourcePath.c_str());
                                        if (m_models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                                        {
                                            candidateName = stem.empty() ? requested : stem;
                                            exists = true;
                                            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Auto-loaded model '%s'", candidateName.c_str());
                                            break;
                                        }
                                    }
                                }
                            }

                            if (!exists)
                            {
                                TraceLog(LOG_WARNING,
                                         "Game::HandleMenuActions() - Model '%s' not available after auto-load attempts; skipping instance for object '%s'",
                                         requested.c_str(), object.name.c_str());
                                continue;
                            }

                            ModelInstanceConfig cfg;
                            cfg.position = object.position;
                            cfg.rotation = object.rotation;
                            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                                            ? object.scale.x : 1.0f;
                            cfg.color = object.color;
                            cfg.spawn = true;

                            if (!m_models->AddInstanceEx(candidateName, cfg))
                            {
                                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Failed to add instance for '%s'", candidateName.c_str());
                            }
                            else
                            {
                                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Added instance for '%s'", candidateName.c_str());
                            }
                        }
                        else if (object.type == MapObjectType::LIGHT)
                        {
                            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Skipping LIGHT object '%s' for model instance creation", object.name.c_str());
                        }
                    }
                }
                else
                {
                    TraceLog(LOG_INFO, "Game::HandleMenuActions() - Detected editor format, using LoadEditorMap");
                    LoadEditorMap(mapPath);
                }
            }
            else
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot open map file: %s", mapPath.c_str());
                throw std::runtime_error("Cannot open map file");
            }

            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Map loaded successfully with %d objects", m_mapManager->GetGameMap().objects.size());
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load map: %s", e.what());
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without map");
            return;
        }

        // Step 5: Initialize player after map is loaded
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing player...");
        try
        {
            InitPlayer();
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized successfully");
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize player: %s", e.what());
            TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Player may not render correctly");
        }

        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Game initialization complete");
        m_isGameInitialized = true;

        // Hide menu and start the game
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Hiding menu and starting game...");
        m_showMenu = false;
        HideCursor();
        m_menu->ResetAction();
    }
    break;

    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Exit game requested from menu.");
        // Clear game state when exiting
        m_menu->SetGameInProgress(false);
        m_showMenu = true; // Show menu one last time before exit
        if (m_engine)
        {
            m_engine->RequestExit();
        }
        m_menu->ResetAction();
        break;

    default:
        break;
    }
}


void Game::RenderGameWorld()
{
    if (!m_engine)
    {
        TraceLog(LOG_WARNING,
                 "Game::RenderGameWorld() - No engine provided, skipping game world render");
        return;
    }

    // Get camera from player
    Camera camera = m_player->GetCamera();

    // Begin 3D rendering
    BeginMode3D(camera);

    // Render editor-created map FIRST (primitives must be rendered before collision shapes)
    // to avoid collision wireframes covering primitives
    if (!m_mapManager->GetGameMap().objects.empty())
    {
        TraceLog(LOG_INFO, "Game::RenderGameWorld() - Rendering map with %d objects",
                 m_mapManager->GetGameMap().objects.size());
        RenderEditorMap();
    }
    else
    {
        TraceLog(LOG_WARNING,
                 "Game::RenderGameWorld() - No map objects to render (m_gameMap.objects.empty())");
    }

    // Render game world (models, player, etc.) and collision shapes AFTER primitives
    // This ensures primitives are visible and not covered by wireframes
    m_engine->GetRenderManager()->RenderGame(*m_player, *m_models, *m_collisionManager,
                                             m_engine->IsCollisionDebugVisible());

    // End 3D rendering
    EndMode3D();
}

// ============================================================================
// Helper Functions
// ============================================================================

void Game::CreatePlatform(const Vector3 &position, const Vector3 &size, Color color,
                          CollisionType collisionType)
{
    m_renderHelper->CreatePlatform(position, size, color, collisionType);
}

float Game::CalculateDynamicFontSize(float baseSize)
{
    return RenderHelper::CalculateDynamicFontSize(baseSize);
}

void Game::RenderGameUI() const
{
    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "Game::RenderGameUI() - No engine provided, skipping game UI render");
        return;
    }

    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player);

    static float gameTime = 0.0f;
    gameTime += GetFrameTime();

    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;
    int milliseconds =
        static_cast<int>((gameTime - static_cast<float>(static_cast<int>(gameTime))) * 1000);

    // Add timer icon using ASCII art timer (works on all systems)
    const char *timerIcon = "[TIMER] ";
    std::string timerText =
        TextFormat("%s%02d:%02d:%03d", timerIcon, minutes, seconds, milliseconds);

    Vector2 timerPos = {300.0f, 20.0f};

    Font fontToUse =
        (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
            ? m_engine->GetRenderManager()->GetFont()
            : GetFontDefault();

    float fontSize = CalculateDynamicFontSize(24.0f);
    DrawTextEx(fontToUse, timerText.c_str(), timerPos, fontSize, 2.0f, WHITE);
}

// ============================================================================
// Editor Map Loading System
// ============================================================================

void Game::LoadEditorMap(const std::string &mapPath)
{
    m_mapManager->LoadEditorMap(mapPath);
}

void Game::RenderEditorMap()
{
    m_mapManager->RenderEditorMap();
}

void Game::DumpMapDiagnostics() const
{
    m_mapManager->DumpMapDiagnostics();
}

void Game::SaveGameState()
{
    if (m_stateManager)
    {
        m_stateManager->SaveGameState(m_mapManager->GetCurrentMapPath());
    }
}

void Game::RestoreGameState()
{
    if (m_stateManager)
    {
        m_stateManager->RestoreGameState();
    }
}

GameMap &Game::GetGameMap()
{
    return m_mapManager->GetGameMap();
}

// Test accessor methods - public for testing purposes

Menu &Game::GetMenu() { return *m_menu; }
