#include "Game.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"
#include "Engine/Engine.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Engine/Map/MapLoader.h"
#include "Game/Menu/Menu.h"
#include "imgui.h"
#include "rlImGui.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>

// Static member definition
Game *Game::s_instance = nullptr;

Game::Game() : m_showMenu(true), m_isGameInitialized(false), m_isDebugInfo(true)
{
    s_instance = this;
    TraceLog(LOG_INFO, "Game class instance created.");
}

/**
 * Cleanup function to properly release resources.
 * Called during game shutdown to ensure clean resource management
 */
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
    if (!m_gameMap.objects.empty())
    {
        m_gameMap.Cleanup();
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
    s_instance = nullptr;
    TraceLog(LOG_INFO, "Game class destructor called.");
    // Note: Cleanup() should be called explicitly before destruction
    // as destructors should not throw exceptions
}

Game *Game::GetInstance() { return s_instance; }

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

    // Initialize engine
    m_engine = std::make_unique<Engine>(config.width, config.height, renderManager, inputManager);
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
    TraceLog(LOG_INFO, "Game::Init() - Menu initialized.");

    TraceLog(LOG_INFO, "Game::Init() - About to initialize kernel...");
    // Kernel boot and service registration
    Kernel &kernel = Kernel::GetInstance();
    kernel.Initialize();
    TraceLog(LOG_INFO, "Game::Init() - Kernel initialized.");

    
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

    kernel.RegisterService<ModelsService>(Kernel::ServiceType::Models,
                                          std::make_shared<ModelsService>(m_models.get()));
    kernel.RegisterService<WorldService>(Kernel::ServiceType::World,
                                         std::make_shared<WorldService>(m_world.get()));
    // CollisionManager and WorldManager will be registered after creation/initialization

    // Models will be loaded selectively when a map is selected
    // LoadGameModels(); // Commented out - replaced with selective loading
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
    Kernel::GetInstance().Update(GetFrameTime());

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
        // Update game logic always, independent of console
        // Debug: Check collision system before updating player
        size_t colliderCount = m_collisionManager->GetColliders().size();
        TraceLog(LOG_INFO, "Game::Update() - Collision system has %d colliders", colliderCount);

        UpdatePlayerLogic();
        UpdatePhysicsLogic();
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
            TraceLog(LOG_INFO, "Game::Render() - Model instances count: %d", currentInstanceCount);
            lastInstanceCount = currentInstanceCount;
        }

        RenderGameWorld();
        RenderGameUI();

        // Add debug info after rendering
        if (!m_gameMap.objects.empty())
        {
            TraceLog(LOG_INFO, "Game::Render() - Rendered %d map objects",
                     m_gameMap.objects.size());
        }
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
    Kernel::GetInstance().Render();
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
    TraceLog(LOG_INFO, "Game::InitCollisions() - Initializing collision system...");

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisions() - Clearing %zu existing colliders (no map loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisions() - Map loaded with %zu existing colliders, preserving them",
                 previousColliderCount);
    }

    // Only create artificial ground if we don't have a custom map with its own ground
    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisions() - No custom map loaded, creating default ground");
        // Ground positioned to align visual model with collision
        Vector3 groundCenter = {0.0f, 0.0f, 0.0f};
        Vector3 groundSize = {1000.0f, 1.0f, 1000.0f};
        Collision groundPlane(groundCenter, groundSize);
        groundPlane.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager->AddCollider(std::move(groundPlane));
    }
    else
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisions() - Custom map loaded, using map's ground objects");
    }

    // Create parkour map based on menu selection
    MenuAction action = m_menu->GetAction();
    switch (action)
    {
    // Map selection now handled dynamically through StartGameWithMap
    case MenuAction::StartGameWithMap:
        m_isGameInitialized = true;
        break;
    default:
        m_isGameInitialized = false;
        break;
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(
        Kernel::ServiceType::Collision,
        std::make_shared<CollisionService>(m_collisionManager.get()));

    // Load model collisions only for models that are actually loaded and required for this map
    auto availableModels = m_models->GetAvailableModels();
    TraceLog(LOG_INFO, "Game::InitCollisions() - Available models for collision generation: %d",
             availableModels.size());
    for (const auto &modelName : availableModels)
    {
        TraceLog(LOG_INFO, "Game::InitCollisions() - Model available: %s", modelName.c_str());
    }

    try
    {
        m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, availableModels);
        TraceLog(LOG_INFO, "Game::InitCollisions() - Model collisions created successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Game::InitCollisions() - Failed to create model collisions: %s",
                 e.what());
        TraceLog(LOG_WARNING, "Game::InitCollisions() - Continuing without model collisions");
    }

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(LOG_INFO, "Game::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());
}

void Game::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "Game::InitCollisionsWithModels() - Initializing collision system with %d required "
             "models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(
            LOG_INFO,
            "Game::InitCollisionsWithModels() - Clearing %zu existing colliders (no map loaded)",
            previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisionsWithModels() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Create parkour map based on menu selection
    MenuAction action = m_menu->GetAction();
    switch (action)
    {
    // Map selection now handled dynamically through StartGameWithMap
    case MenuAction::StartGameWithMap:
        m_isGameInitialized = true;
        break;
    default:
        m_isGameInitialized = true;
        break;
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(
        Kernel::ServiceType::Collision,
        std::make_shared<CollisionService>(m_collisionManager.get()));

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "Game::InitCollisionsWithModels() - Required models for collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Model required: %s",
                 modelName.c_str());
    }

    try
    {
        m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
        TraceLog(LOG_INFO,
                 "Game::InitCollisionsWithModels() - Model collisions created successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "Game::InitCollisionsWithModels() - Failed to create model collisions: %s",
                 e.what());
        TraceLog(LOG_WARNING,
                 "Game::InitCollisionsWithModels() - Continuing without model collisions");
    }

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(LOG_INFO,
             "Game::InitCollisionsWithModels() - Collision system initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());
}

bool Game::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "Game::InitCollisionsWithModelsSafe() - Initializing collision system with %d "
             "required models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisionsWithModelsSafe() - Clearing %zu existing colliders (no map "
                 "loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "Game::InitCollisionsWithModelsSafe() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize collision manager
    m_collisionManager->Initialize();

    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(
        Kernel::ServiceType::Collision,
        std::make_shared<CollisionService>(m_collisionManager.get()));

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "Game::InitCollisionsWithModelsSafe() - Required models for collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModelsSafe() - Model required: %s",
                 modelName.c_str());
    }

    // Try to create model collisions safely
    try
    {
        m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
        TraceLog(LOG_INFO,
                 "Game::InitCollisionsWithModelsSafe() - Model collisions created successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_WARNING,
                 "Game::InitCollisionsWithModelsSafe() - Failed to create model collisions: %s",
                 e.what());
        TraceLog(LOG_WARNING,
                 "Game::InitCollisionsWithModelsSafe() - Continuing with basic collision system");
    }
    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(
        LOG_INFO,
        "Game::InitCollisionsWithModelsSafe() - Collision system initialized with %zu colliders.",
        m_collisionManager->GetColliders().size());

    return true; // Always return true since we have at least basic collision
}

void Game::InitPlayer()
{
    TraceLog(LOG_INFO, "Game::InitPlayer() - Initializing player...");

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
            if (!m_models->LoadSingleModel("player", "../resources/player_low.glb", true))
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
        m_player->SetPlayerPosition(
            {currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    else if (currentPos.y > 50.0f)
    {
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player position too high, adjusting");
        m_player->SetPlayerPosition(
            {currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }

    // Check if map has PlayerStart objects and adjust position accordingly
    TraceLog(LOG_INFO, "Game::InitPlayer() - Checking for PlayerStart objects in map...");
    if (!m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO, "Game::InitPlayer() - Map has %d objects, searching for PlayerStart...",
                 m_gameMap.objects.size());
        for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
        {
            const auto &obj = m_gameMap.objects[i];
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
    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading game models from resources directory...");
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(true);
    m_models->SetSelectiveMode(false);

    MapLoader mapLoader;
    std::string resourcesDir = std::string(PROJECT_ROOT_DIR) + "/resources";
    auto models = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (models.empty())
    {
        TraceLog(LOG_WARNING, "Game::LoadGameModels() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO, "Game::LoadGameModels() - Found %d models in resources directory",
             models.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(models.size()), // totalModels
        0,                               // loadedModels
        0,                               // failedModels
        0.0f                             // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load each model found in the directory
    for (const auto &modelInfo : models)
    {
        std::string modelPath = modelInfo.path;
        TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading model: %s from %s",
                 modelInfo.name.c_str(), modelPath.c_str());

        if (m_models->LoadSingleModel(modelInfo.name, modelPath, true))
        {
            result.loadedModels++;
            TraceLog(LOG_INFO, "Successfully loaded model: %s", modelInfo.name.c_str());
        }
        else
        {
            result.failedModels++;
            TraceLog(LOG_WARNING, "Failed to load model: %s", modelInfo.name.c_str());
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player_low") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(
            LOG_WARNING,
            "Game::LoadGameModels() - Player model not found, player may not render correctly");
    }

    return result;
}

std::optional<ModelLoader::LoadResult>
Game::LoadGameModelsSelective(const std::vector<std::string> &modelNames)
{
    TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Loading selective models: %d models",
             modelNames.size());
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(false);
    m_models->SetSelectiveMode(true);

    MapLoader mapLoader;
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    auto allModels = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (allModels.empty())
    {
        TraceLog(LOG_WARNING,
                 "Game::LoadGameModelsSelective() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Found %d models in resources directory",
             allModels.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(modelNames.size()), // totalModels (only count requested models)
        0,                                   // loadedModels
        0,                                   // failedModels
        0.0f                                 // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load only the models that are in the required list
    for (const auto &modelName : modelNames)
    {
        auto it =
            std::find_if(allModels.begin(), allModels.end(),
                         [&modelName](const ModelInfo &info) { return info.name == modelName; });

        if (it != allModels.end())
        {
            std::string modelPath = it->path;
            TraceLog(LOG_INFO,
                     "Game::LoadGameModelsSelective() - Loading required model: %s from %s",
                     modelName.c_str(), modelPath.c_str());

            if (m_models->LoadSingleModel(modelName, modelPath, true))
            {
                result.loadedModels++;
                TraceLog(LOG_INFO, "Successfully loaded model: %s", modelName.c_str());
            }
            else
            {
                result.failedModels++;
                TraceLog(LOG_WARNING, "Failed to load model: %s", modelName.c_str());
            }
        }
        else
        {
            TraceLog(LOG_WARNING,
                     "Game::LoadGameModelsSelective() - Model not found in resources: %s",
                     modelName.c_str());
            result.failedModels++;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(LOG_WARNING, "Game::LoadGameModelsSelective() - Player model not found, player "
                              "may not render correctly");
    }

    return result;
}

std::optional<ModelLoader::LoadResult>
Game::LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames)
{
    TraceLog(LOG_INFO, "Game::LoadGameModelsSelectiveSafe() - Loading selective models: %d models",
             modelNames.size());
    m_models->SetCacheEnabled(true);
    m_models->SetMaxCacheSize(50);
    m_models->EnableLOD(false);
    m_models->SetSelectiveMode(true);

    MapLoader mapLoader;
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    auto allModels = mapLoader.LoadModelsFromDirectory(resourcesDir);

    if (allModels.empty())
    {
        TraceLog(LOG_WARNING,
                 "Game::LoadGameModelsSelectiveSafe() - No models found in resources directory");
        return std::nullopt;
    }

    TraceLog(LOG_INFO,
             "Game::LoadGameModelsSelectiveSafe() - Found %d models in resources directory",
             allModels.size());

    ModelLoader::LoadResult result = {
        static_cast<int>(modelNames.size()), // totalModels (only count requested models)
        0,                                   // loadedModels
        0,                                   // failedModels
        0.0f                                 // loadingTime
    };

    auto startTime = std::chrono::steady_clock::now();

    // Load only the models that are in the required list
    std::unordered_set<std::string> modelNameSet(modelNames.begin(), modelNames.end());

    for (const auto &modelInfo : allModels)
    {
        if (modelNameSet.find(modelInfo.name) != modelNameSet.end())
        {
            TraceLog(LOG_INFO,
                     "Game::LoadGameModelsSelectiveSafe() - Loading required model: %s from %s",
                     modelInfo.name.c_str(), modelInfo.path.c_str());

            if (m_models->LoadSingleModel(modelInfo.name, modelInfo.path, true))
            {
                result.loadedModels++;
                TraceLog(LOG_INFO, "Successfully loaded model: %s", modelInfo.name.c_str());
            }
            else
            {
                result.failedModels++;
                TraceLog(LOG_WARNING, "Failed to load model: %s", modelInfo.name.c_str());
            }
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    result.loadingTime = std::chrono::duration<float>(endTime - startTime).count();

    m_models->PrintStatistics();
    TraceLog(LOG_INFO, "Game::LoadGameModelsSelectiveSafe() - Loaded %d/%d models in %.2f seconds",
             result.loadedModels, result.totalModels, result.loadingTime);

    // Validate that we have essential models
    auto availableModels = m_models->GetAvailableModels();
    bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") !=
                          availableModels.end();

    if (!hasPlayerModel)
    {
        TraceLog(LOG_WARNING, "Game::LoadGameModelsSelectiveSafe() - Player model not found, "
                              "player may not render correctly");
    }

    return result;
}

///
/// Maps object types to appropriate model names for selective loading
/// @param objectType The MapObjectType enum value
/// @param modelName The specific model name from the map object (if any)
/// @return Model name if mapping exists, empty string otherwise
///
std::string Game::GetModelNameForObjectType(int objectType, const std::string &modelName)
{
    // Cast to MapObjectType enum for better readability
    MapObjectType type = static_cast<MapObjectType>(objectType);

    switch (type)
    {
    case MapObjectType::MODEL:
    {
        // For MODEL type objects, return the actual model name if provided
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - MODEL object requires model: %s",
                     modelName.c_str());
            return modelName;
        }
        // Fallback: try to infer model from common naming patterns
        TraceLog(LOG_WARNING,
                 "Game::GetModelNameForObjectType() - MODEL object has no modelName specified");
        return "";
    }

    case MapObjectType::LIGHT:
    {
        // Handle incorrectly exported MODEL objects as LIGHT type
        // This is a known issue with the map editor
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - LIGHT object (likely MODEL) requires "
                     "model: %s",
                     modelName.c_str());
            return modelName;
        }
        // LIGHT objects don't typically require 3D models for rendering
        return "";
    }

    case MapObjectType::CUBE:
    {
        // Cubes are rendered as primitives, no model needed
        // But some maps might have custom cube models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - CUBE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::SPHERE:
    {
        // Spheres are rendered as primitives, no model needed
        // But some maps might have custom sphere models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - SPHERE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::CYLINDER:
    {
        // Cylinders are rendered as primitives, no model needed
        // But some maps might have custom cylinder models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - CYLINDER object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    case MapObjectType::PLANE:
    {
        // Planes are rendered as primitives, no model needed
        // But some maps might have custom plane models
        if (!modelName.empty())
        {
            TraceLog(LOG_DEBUG,
                     "Game::GetModelNameForObjectType() - PLANE object with custom model: %s",
                     modelName.c_str());
            return modelName;
        }
        return "";
    }

    default:
    {
        TraceLog(LOG_WARNING, "Game::GetModelNameForObjectType() - Unknown object type: %d",
                 objectType);
        return "";
    }
    }
}

std::vector<std::string> Game::GetModelsRequiredForMap(const std::string &mapIdentifier)
{
    std::vector<std::string> requiredModels;

    // Always include the player model as it's essential for gameplay
    requiredModels.emplace_back("player");

    // Convert map name to full path if needed
    std::string mapPath = mapIdentifier;
    if (mapPath.substr(mapPath.find_last_of('.') + 1) != "json")
    {
        // If it's not a path ending in .json, assume it's a map name and construct the path
        mapPath = PROJECT_ROOT_DIR "/resources/maps/" + mapIdentifier;
        if (mapIdentifier.find(".json") == std::string::npos)
        {
            mapPath += ".json";
        }
    }

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    if (extension == "json")
    {
        TraceLog(LOG_INFO,
                 "Game::GetModelsRequiredForMap() - Analyzing JSON map for model requirements: %s",
                 mapPath.c_str());

        std::ifstream file(mapPath);
        if (file.is_open())
        {
            try
            {
                // Read entire file content for manual parsing
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                file.close();

                // Check if this is the editor format or game format (direct array)
                // Editor format: {"objects": [...] ...}
                // Game format: [...]
                size_t objectsStart = content.find("\"objects\"");
                size_t arrayStart = content.find("[");

                if (objectsStart != std::string::npos)
                {
                    // This is the editor format with metadata - use nlohmann/json
                    json j = json::parse(content);

                    // Look for objects with model references
                    if (j.contains("objects") && j["objects"].is_array())
                    {
                        for (const auto &object : j["objects"])
                        {
                            // Extract object type and model name
                            int objectType = -1;
                            std::string objectModelName = "";

                            if (object.contains("type") && object["type"].is_number_integer())
                            {
                                objectType = object["type"].get<int>();
                            }

                            if (object.contains("modelName") && object["modelName"].is_string())
                            {
                                objectModelName = object["modelName"].get<std::string>();
                            }

                            // Use our improved function to determine if this object needs a model
                            std::string modelName =
                                GetModelNameForObjectType(objectType, objectModelName);

                            // Add model to requirements if found and not already in list
                            if (!modelName.empty())
                            {
                                if (std::find(requiredModels.begin(), requiredModels.end(),
                                              modelName) == requiredModels.end())
                                {
                                    requiredModels.push_back(modelName);
                                    TraceLog(LOG_INFO,
                                             "Game::GetModelsRequiredForMap() - Object type %d "
                                             "requires model: %s",
                                             objectType, modelName.c_str());
                                }
                                else
                                {
                                    TraceLog(LOG_DEBUG,
                                             "Game::GetModelsRequiredForMap() - Model %s already "
                                             "in requirements list",
                                             modelName.c_str());
                                }
                            }
                            else if (objectType != -1)
                            {
                                TraceLog(LOG_DEBUG,
                                         "Game::GetModelsRequiredForMap() - Object type %d does "
                                         "not require a model",
                                         objectType);
                            }
                        }
                    }
                }
                else if (arrayStart != std::string::npos)
                {
                    // This is the game format (direct array) - parse manually
                    TraceLog(
                        LOG_INFO,
                        "Game::GetModelsRequiredForMap() - Detected game format, parsing manually");

                    // Find all objects in the array
                    size_t pos = arrayStart + 1;
                    std::string objectStartStr = "{";
                    size_t objectStart = content.find(objectStartStr, pos);

                    while (objectStart != std::string::npos)
                    {
                        // Find the matching closing brace
                        size_t braceCount = 0;
                        size_t objectEnd = objectStart;

                        while (objectEnd < content.length())
                        {
                            if (content[objectEnd] == '{')
                                braceCount++;
                            else if (content[objectEnd] == '}')
                            {
                                braceCount--;
                                if (braceCount == 0)
                                    break;
                            }
                            objectEnd++;
                        }

                        if (objectEnd < content.length())
                        {
                            std::string objectJson =
                                content.substr(objectStart, objectEnd - objectStart + 1);

                            // Parse modelPath field manually
                            size_t modelPathPos = objectJson.find("\"modelPath\"");
                            if (modelPathPos != std::string::npos)
                            {
                                size_t quoteStart = objectJson.find('\"', modelPathPos + 11);
                                if (quoteStart != std::string::npos)
                                {
                                    size_t quoteEnd = objectJson.find('\"', quoteStart + 1);
                                    if (quoteEnd != std::string::npos)
                                    {
                                        std::string modelName = objectJson.substr(
                                            quoteStart + 1, quoteEnd - quoteStart - 1);
                                        if (!modelName.empty())
                                        {
                                            // Check if this model is not already in the list
                                            if (std::find(requiredModels.begin(),
                                                          requiredModels.end(),
                                                          modelName) == requiredModels.end())
                                            {
                                                requiredModels.push_back(modelName);
                                                TraceLog(LOG_INFO,
                                                         "Game::GetModelsRequiredForMap() - Found "
                                                         "model requirement: %s",
                                                         modelName.c_str());
                                            }
                                        }
                                    }
                                }
                            }
                            // Also check for modelName field in game format
                            else
                            {
                                size_t modelNamePos = objectJson.find("\"modelName\"");
                                if (modelNamePos != std::string::npos)
                                {
                                    size_t quoteStart = objectJson.find('\"', modelNamePos + 12);
                                    if (quoteStart != std::string::npos)
                                    {
                                        size_t quoteEnd = objectJson.find('\"', quoteStart + 1);
                                        if (quoteEnd != std::string::npos)
                                        {
                                            std::string modelName = objectJson.substr(
                                                quoteStart + 1, quoteEnd - quoteStart - 1);
                                            if (!modelName.empty())
                                            {
                                                // Check if this model is not already in the list
                                                if (std::find(requiredModels.begin(),
                                                              requiredModels.end(),
                                                              modelName) == requiredModels.end())
                                                {
                                                    requiredModels.push_back(modelName);
                                                    TraceLog(
                                                        LOG_INFO,
                                                        "Game::GetModelsRequiredForMap() - Found "
                                                        "model requirement in game format: %s",
                                                        modelName.c_str());
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            pos = objectEnd + 1;
                            objectStart = content.find(objectStartStr, pos);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING, "Game::GetModelsRequiredForMap() - No valid JSON "
                                          "structure found in map file");
                }
            }
            catch (const std::exception &e)
            {
                TraceLog(LOG_WARNING,
                         "Game::GetModelsRequiredForMap() - Error parsing map JSON: %s", e.what());
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Game::GetModelsRequiredForMap() - Could not open map file: %s",
                     mapPath.c_str());
        }
    }

    TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Total models required: %d",
             requiredModels.size());
    return requiredModels;
}

void Game::UpdatePlayerLogic()
{
    if (!m_engine)
    {
        // Skip player logic if no engine is available (for testing)
        TraceLog(LOG_INFO, "Game::UpdatePlayerLogic() - No engine, updating player");
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
        TraceLog(LOG_INFO,
                 "Game::UpdatePlayerLogic() - ImGui capturing mouse, only updating camera");
        // return;
    }

    Vector3 posBefore = m_player->GetPlayerPosition();
    Vector3 velBefore = m_player->GetPhysics().GetVelocity();
    TraceLog(LOG_INFO,
             "Game::UpdatePlayerLogic() - Before update: position (%.3f, %.3f, %.3f), velocity "
             "(%.3f, %.3f, %.3f)",
             posBefore.x, posBefore.y, posBefore.z, velBefore.x, velBefore.y, velBefore.z);

    m_player->Update(*m_collisionManager);

    Vector3 posAfter = m_player->GetPlayerPosition();
    Vector3 velAfter = m_player->GetPhysics().GetVelocity();
    TraceLog(LOG_INFO,
             "Game::UpdatePlayerLogic() - After update: position (%.3f, %.3f, %.3f), velocity "
             "(%.3f, %.3f, %.3f)",
             posAfter.x, posAfter.y, posAfter.z, velAfter.x, velAfter.y, velAfter.z);

    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player);
}

/**
 * Updates physics-related game logic.
 *
 * Ensures collision system is properly initialized and handles
 * edge cases where collision data might be missing.
 */
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
        try
        {
            if (m_gameMap.objects.empty())
            {
                // Emergency ground plane creation removed - GroundColliderFactory no longer exists
                // Collision plane = GroundColliderFactory::CreateDefaultGameGround();
                // m_collisionManager.AddCollider(std::move(plane));
                TraceLog(LOG_WARNING,
                         "Game::UpdatePhysicsLogic() - Created emergency ground plane.");
            }
            else
            {
                TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - No colliders but custom map "
                                      "loaded, using map objects for collision.");
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR,
                     "Game::UpdatePhysicsLogic() - Failed to create emergency ground plane: %s",
                     e.what());
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
            std::vector<std::string> requiredModels = GetModelsRequiredForMap(m_savedMapPath);
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
                std::vector<std::string> requiredModels = GetModelsRequiredForMap(m_savedMapPath);

                // Reinitialize collision system safely
                try
                {
                    // Clear existing colliders
                    m_collisionManager->ClearColliders();

                    // Create ground collision first (only if no custom map)
                    if (m_gameMap.objects.empty())
                    {
                        // Ground positioned to align visual model with collision
                        Vector3 groundCenter = {0.0f, 0.0f, 0.0f};
                        Vector3 groundSize = {1000.0f, 1.0f, 1000.0f};
                        Collision groundPlane(groundCenter, groundSize);
                        groundPlane.SetCollisionType(CollisionType::AABB_ONLY);
                        m_collisionManager->AddCollider(std::move(groundPlane));
                    }

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
                requiredModels.emplace_back("player"); // Always include player model
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
                    m_gameMap = LoadGameMap(mapPath.c_str());

                    // Register any models that MapLoader preloaded into the GameMap
                    if (!m_gameMap.loadedModels.empty())
                    {
                        TraceLog(LOG_INFO,
                                 "Game::HandleMenuActions() - Registering %d preloaded models from map into ModelLoader",
                                 m_gameMap.loadedModels.size());
                        for (const auto &p : m_gameMap.loadedModels)
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
                             m_gameMap.objects.size());
                    for (const auto &object : m_gameMap.objects)
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

            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Map loaded successfully with %d objects", m_gameMap.objects.size());
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

    // Render game world (models, player, etc.)
    m_engine->GetRenderManager()->RenderGame(*m_player, *m_models, *m_collisionManager,
                                             m_engine->IsCollisionDebugVisible());

    // Render editor-created map if available - MUST be inside 3D context
    if (!m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO, "Game::RenderGameWorld() - Rendering map with %d objects",
                 m_gameMap.objects.size());
        RenderEditorMap();
    }
    else
    {
        TraceLog(LOG_WARNING,
                 "Game::RenderGameWorld() - No map objects to render (m_gameMap.objects.empty())");
    }

    // End 3D rendering
    EndMode3D();
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Creates a platform with collision box at specified position
 * @param position Platform center position in 3D space
 * @param size Platform dimensions (width, height, depth)
 * @param color Platform render color
 * @param collisionType Type of collision detection to use
 *
 * This helper function reduces code duplication in map creation functions
 * and ensures consistent platform creation across all map types.
 */
void Game::CreatePlatform(const Vector3 &position, const Vector3 &size, Color color,
                          CollisionType collisionType)
{
    DrawCube(position, size.x, size.y, size.z, color);

    Collision collision(position, size);
    collision.SetCollisionType(collisionType);
    m_collisionManager->AddCollider(std::move(collision));
}

/**
 * Calculates dynamic font size based on screen resolution
 * @param baseSize Base font size for 1920p resolution
 * @return Scaled font size clamped to reasonable bounds
 */
float Game::CalculateDynamicFontSize(float baseSize)
{
    int screenWidth = GetScreenWidth();
    float scaleFactor = static_cast<float>(screenWidth) / 1920.0f;
    float dynamicSize = baseSize * scaleFactor;

    // Clamp to reasonable bounds
    return std::max(18.0f, std::min(48.0f, dynamicSize));
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
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Loading map from: %s", mapPath.c_str());
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Debug: Map path logged for verification");

    // Validate map path exists and is readable
    if (!std::filesystem::exists(mapPath))
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Map file does not exist: %s", mapPath.c_str());
        return;
    }

    // Clear previous map data
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Clearing previous map data...");
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Current collider count before map load: %zu",
             m_collisionManager->GetColliders().size());
    m_gameMap.Cleanup();
    m_gameMap = GameMap{};

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - File extension: %s", extension.c_str());

    if (extension == "json")
    {
        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Detected JSON format, using MapLoader");

        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Using MapEditor loader (JsonMapFileManager) "
                           "for JSON parsing...");
        // Use Map Editor's loader directly and convert its objects into GameMap
        std::vector<JsonSerializableObject> editorObjects;
        MapMetadata editorMeta;
        if (!JsonMapFileManager::LoadMap(editorObjects, mapPath, editorMeta))
        {
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - JsonMapFileManager failed to load map: %s",
                     mapPath.c_str());
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - This may indicate JSON parsing errors or "
                                "invalid map format");
            return;
        }

        TraceLog(LOG_INFO,
                 "Game::LoadEditorMap() - JsonMapFileManager loaded %zu objects successfully",
                 editorObjects.size());

        GameMap adapterMap;
        adapterMap.metadata = editorMeta;

        for (const auto &eo : editorObjects)
        {
            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - Processing editor object: name='%s', type=%d, "
                     "modelName='%s'",
                     eo.name.c_str(), eo.type, eo.modelName.c_str());

            MapObjectData od;
            od.name =
                eo.name.empty() ? ("object_" + std::to_string(adapterMap.objects.size())) : eo.name;
            od.type = static_cast<MapObjectType>(eo.type);
            od.position = eo.position;
            od.rotation = eo.rotation;
            od.scale = eo.scale;
            od.color = eo.color;
            od.modelName = eo.modelName;
            od.radius = eo.radiusSphere;
            od.height = eo.radiusV;
            od.size = eo.size;
            od.isPlatform = true;
            od.isObstacle = false;

            // Sanitize values coming from editor JSON to avoid NaN/Inf propagating to render
            auto sanitizeVec3 = [](Vector3 v)
            {
                if (!std::isfinite(v.x))
                    v.x = 0.0f;
                if (!std::isfinite(v.y))
                    v.y = 0.0f;
                if (!std::isfinite(v.z))
                    v.z = 0.0f;
                return v;
            };
            auto sanitizeFloat = [](float f, float fallback)
            { return std::isfinite(f) ? f : fallback; };

            od.position = sanitizeVec3(od.position);
            od.rotation = sanitizeVec3(od.rotation);
            od.scale = sanitizeVec3(od.scale);
            if (od.scale.x == 0.0f && od.scale.y == 0.0f && od.scale.z == 0.0f)
            {
                od.scale = {1.0f, 1.0f, 1.0f};
            }
            od.radius = sanitizeFloat(od.radius, 1.0f);
            od.height = sanitizeFloat(od.height, 1.0f);
            if (!std::isfinite(od.size.x))
                od.size.x = 0.0f;
            if (!std::isfinite(od.size.y))
                od.size.y = 0.0f;

            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - Sanitized object: pos=(%.2f,%.2f,%.2f), "
                     "scale=(%.2f,%.2f,%.2f), radius=%.2f",
                     od.position.x, od.position.y, od.position.z, od.scale.x, od.scale.y,
                     od.scale.z, od.radius);

            adapterMap.objects.push_back(od);

            // If this object references a model, attempt to preload it into the
            // GameMap.loadedModels
            if (od.type == MapObjectType::MODEL && !od.modelName.empty())
            {
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Attempting to load model: %s",
                         od.modelName.c_str());

                // Try multiple path variations for the model
                std::vector<std::string> possiblePaths;
                std::string modelName = od.modelName;

                // Remove extension if present to try different extensions
                std::string stem = std::filesystem::path(modelName).stem().string();
                std::string extension = std::filesystem::path(modelName).extension().string();

                // Build possible paths
                if (extension.empty())
                {
                    // No extension provided, try common extensions
                    std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
                    for (const auto &ext : extensions)
                    {
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                modelName + ext);
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                stem + ext);
                    }
                }
                else
                {
                    // Extension provided, use as-is
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                            modelName);
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem +
                                            extension);
                }

                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Trying %zu possible paths for model %s",
                         possiblePaths.size(), od.modelName.c_str());

                // Try to find and load the model
                bool modelLoaded = false;
                for (const auto &modelPath : possiblePaths)
                {
                    TraceLog(LOG_DEBUG, "Game::LoadEditorMap() - Checking path: %s",
                             modelPath.c_str());
                    if (std::ifstream(modelPath).good())
                    {
                        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Found model file at: %s",
                                 modelPath.c_str());
                        // Avoid duplicate loads
                        if (adapterMap.loadedModels.find(od.modelName) ==
                            adapterMap.loadedModels.end())
                        {
                            Model model = ::LoadModel(modelPath.c_str());
                            if (model.meshCount > 0)
                            {
                                adapterMap.loadedModels[od.modelName] = model;
                                TraceLog(LOG_INFO,
                                         "Game::LoadEditorMap() - Successfully loaded model %s "
                                         "from %s (meshCount: %d)",
                                         od.modelName.c_str(), modelPath.c_str(), model.meshCount);
                                modelLoaded = true;
                                break;
                            }
                            else
                            {
                                TraceLog(
                                    LOG_WARNING,
                                    "Game::LoadEditorMap() - Model loaded but has no meshes: %s",
                                    modelPath.c_str());
                            }
                        }
                        else
                        {
                            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Model %s already loaded",
                                     od.modelName.c_str());
                            modelLoaded = true;
                            break;
                        }
                    }
                }

                if (!modelLoaded)
                {
                    TraceLog(LOG_ERROR,
                             "Game::LoadEditorMap() - CRITICAL: Could not find or load model file "
                             "for %s. This will cause rendering failures.",
                             od.modelName.c_str());
                    TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Tried paths:");
                    for (const auto &path : possiblePaths)
                    {
                        TraceLog(LOG_ERROR, "  - %s", path.c_str());
                    }
                }
            }
        }

        m_gameMap = std::move(adapterMap);

        if (!m_gameMap.objects.empty())
        {
            // Register any models preloaded by MapLoader into the runtime ModelLoader
            if (!m_gameMap.loadedModels.empty())
            {
                TraceLog(LOG_INFO,
                         "Game::LoadEditorMap() - Registering %d preloaded models from map into "
                         "ModelLoader",
                         m_gameMap.loadedModels.size());
                for (const auto &p : m_gameMap.loadedModels)
                {
                    const std::string &modelName = p.first;
                    const ::Model &loaded = p.second;

                    // Validate model before registration
                    if (loaded.meshCount > 0)
                    {
                        if (m_models->RegisterLoadedModel(modelName, loaded))
                        {
                            TraceLog(LOG_INFO,
                                     "Game::LoadEditorMap() - Successfully registered model from "
                                     "map: %s (meshCount: %d)",
                                     modelName.c_str(), loaded.meshCount);
                        }
                        else
                        {
                            TraceLog(
                                LOG_WARNING,
                                "Game::LoadEditorMap() - Failed to register model from map: %s",
                                modelName.c_str());
                        }
                    }
                    else
                    {
                        TraceLog(LOG_WARNING,
                                 "Game::LoadEditorMap() - Skipping invalid model from map: %s "
                                 "(meshCount: %d)",
                                 modelName.c_str(), loaded.meshCount);
                    }
                }
            }
            else
            {
                TraceLog(LOG_INFO,
                         "Game::LoadEditorMap() - No preloaded models in GameMap to register");
            }

            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - MapLoader import successful, processing %d objects",
                     m_gameMap.objects.size());

            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - Successfully loaded JSON map with %d objects",
                     m_gameMap.objects.size());
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Debug: Object count verified as %d",
                     m_gameMap.objects.size());
        }
        else
        {
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Failed to load JSON map");
            return;
        }
    }

    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Map loaded, checking object count: %d",
             m_gameMap.objects.size());
    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Validate map object count to prevent memory issues
    if (m_gameMap.objects.size() > 10000)
    {
        TraceLog(LOG_ERROR,
                 "Game::LoadEditorMap() - Map has too many objects (%d), limiting to 10000",
                 m_gameMap.objects.size());
        return;
    }

    // Create collision boxes for all objects in the map
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Creating collision boxes for %d objects",
             m_gameMap.objects.size());
    size_t collisionCreationCount = 0;
    size_t collisionSkippedCount = 0;

    for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
    {
        const auto &object = m_gameMap.objects[i];

        // Debug log for each object's details
        TraceLog(LOG_INFO,
                 "Game::LoadEditorMap() - Object %d details: name='%s', type=%d, modelName='%s', "
                 "position=(%.2f,%.2f,%.2f), scale=(%.2f,%.2f,%.2f), color=(%d,%d,%d,%d)",
                 i, object.name.c_str(), static_cast<int>(object.type), object.modelName.c_str(),
                 object.position.x, object.position.y, object.position.z, object.scale.x,
                 object.scale.y, object.scale.z, object.color.r, object.color.g, object.color.b,
                 object.color.a);

        // Validate object data before creating collision
        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) ||
            !std::isfinite(object.position.z))
        {
            TraceLog(LOG_WARNING,
                     "Game::LoadEditorMap() - Object %d has invalid position, skipping collision",
                     i);
            collisionSkippedCount++;
            continue;
        }

        if (!std::isfinite(object.scale.x) || !std::isfinite(object.scale.y) ||
            !std::isfinite(object.scale.z))
        {
            TraceLog(LOG_WARNING,
                     "Game::LoadEditorMap() - Object %d has invalid scale, skipping collision", i);
            collisionSkippedCount++;
            continue;
        }

        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Creating collision for object %d: %s", i,
                 object.name.c_str());
        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Object %d position: (%.2f, %.2f, %.2f)", i,
                 object.position.x, object.position.y, object.position.z);

        Vector3 colliderSize = object.scale;

        // Adjust collider size based on object type
        switch (object.type)
        {
        case MapObjectType::SPHERE:
            // For spheres, use radius for all dimensions
            {
                float radius = object.radius > 0.0f ? object.radius : 1.0f;
                colliderSize = Vector3{radius, radius, radius};
            }
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Sphere collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::CYLINDER:
            // For cylinders, use radius for x/z and height for y
            {
                float radius = object.radius > 0.0f ? object.radius : 1.0f;
                float height = object.height > 0.0f ? object.height : 2.0f;
                colliderSize = Vector3{radius, height, radius};
            }
            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - Cylinder collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::PLANE:
            // For planes, use size for x/z and small height for y
            colliderSize = Vector3{object.size.x, 0.1f, object.size.y};
            // Fallback to default if size is zero
            if (colliderSize.x == 0.0f)
                colliderSize.x = 5.0f;
            if (colliderSize.z == 0.0f)
                colliderSize.z = 5.0f;
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Plane collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::MODEL:
            // For models, use scale as bounding box - scale represents the size of the model
            // instance
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Model collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::LIGHT:
            // LIGHT objects don't need collision - they are just lighting
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - LIGHT object: skipping collision creation");
            collisionSkippedCount++;
            continue; // Skip collision creation for LIGHT objects
        default:
            // For cubes and other types, use scale as-is
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Cube collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        }

        // Ensure colliderSize has valid non-zero dimensions
        if (colliderSize.x == 0.0f)
            colliderSize.x = 1.0f;
        if (colliderSize.y == 0.0f)
            colliderSize.y = 1.0f;
        if (colliderSize.z == 0.0f)
            colliderSize.z = 1.0f;

        // Validate final collider size
        if (!std::isfinite(colliderSize.x) || !std::isfinite(colliderSize.y) ||
            !std::isfinite(colliderSize.z))
        {
            TraceLog(LOG_WARNING,
                     "Game::LoadEditorMap() - Object %d has invalid colliderSize after "
                     "calculation, skipping collision",
                     i);
            continue;
        }

        TraceLog(LOG_INFO,
                 "Game::LoadEditorMap() - Final colliderSize for object %d: (%.2f, %.2f, %.2f)", i,
                 colliderSize.x, colliderSize.y, colliderSize.z);

        try
        {
            Collision collision(object.position, colliderSize);
            collision.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager->AddCollider(std::move(collision));

            TraceLog(LOG_INFO,
                     "Game::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f) with "
                     "size (%.2f, %.2f, %.2f)",
                     object.name.c_str(), object.position.x, object.position.y, object.position.z,
                     colliderSize.x, colliderSize.y, colliderSize.z);
            collisionCreationCount++;
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR,
                     "Game::LoadEditorMap() - Failed to create collision for object %s: %s",
                     object.name.c_str(), e.what());
            collisionSkippedCount++;
        }
    }

    // Set player start position if specified in map metadata
    if (m_gameMap.metadata.startPosition.x != 0.0f || m_gameMap.metadata.startPosition.y != 0.0f ||
        m_gameMap.metadata.startPosition.z != 0.0f)
    {
        m_player->SetPlayerPosition(m_gameMap.metadata.startPosition);
        TraceLog(LOG_INFO,
                 "Game::LoadEditorMap() - Set player start position to (%.2f, %.2f, %.2f)",
                 m_gameMap.metadata.startPosition.x, m_gameMap.metadata.startPosition.y,
                 m_gameMap.metadata.startPosition.z);
    }

    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Successfully loaded map with %d objects",
             m_gameMap.objects.size());
    TraceLog(LOG_INFO,
             "Game::LoadEditorMap() - Collision creation summary: %zu created, %zu skipped",
             collisionCreationCount, collisionSkippedCount);
    TraceLog(LOG_INFO,
             "Game::LoadEditorMap() - Final collider count after creating collisions: %zu",
             m_collisionManager->GetColliders().size());

    // Log object types breakdown
    int modelObjects = 0, lightObjects = 0, cubeObjects = 0, otherObjects = 0;
    for (const auto &obj : m_gameMap.objects)
    {
        if (obj.type == MapObjectType::MODEL)
            modelObjects++;
        else if (obj.type == MapObjectType::LIGHT)
            lightObjects++;
        else if (obj.type == MapObjectType::CUBE)
            cubeObjects++;
        else
            otherObjects++;
    }
    TraceLog(LOG_INFO,
             "Game::LoadEditorMap() - Object types: %d MODEL, %d LIGHT, %d CUBE, %d other",
             modelObjects, lightObjects, cubeObjects, otherObjects);

    // Validate critical resources
    if (modelObjects > 0 && m_gameMap.loadedModels.empty())
    {
        TraceLog(
            LOG_WARNING,
            "Game::LoadEditorMap() - WARNING: Map has %d model objects but no models were loaded!",
            modelObjects);
    }

    if (collisionCreationCount == 0)
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - CRITICAL: No collisions were created for the "
                            "map! Physics will not work.");
    }

    // Dump diagnostics to help find why instances are not created
    DumpMapDiagnostics();

    // Create model instances in the ModelLoader for all MODEL objects so they are drawn
    TraceLog(LOG_INFO,
             "Game::LoadEditorMap() - Creating model instances for %d objects if applicable",
             m_gameMap.objects.size());
    // First, ensure that all referenced model files are registered in the ModelLoader.
    // Some maps may reference model names (stems) or filenames with extensions — try common
    // extensions.
    std::set<std::string> uniqueModelNames;
    for (const auto &object : m_gameMap.objects)
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
            uniqueModelNames.insert(object.modelName);
    }

    auto available = m_models->GetAvailableModels();
    for (const auto &requested : uniqueModelNames)
    {
        if (std::find(available.begin(), available.end(), requested) != available.end())
            continue; // already present

        // Try stem (strip extension)
        std::string stem = std::filesystem::path(requested).stem().string();
        if (!stem.empty() && std::find(available.begin(), available.end(), stem) != available.end())
            continue; // present as stem

        // Attempt to auto-load from resources using robust path resolution
        std::vector<std::string> possiblePaths;
        std::string extension = std::filesystem::path(requested).extension().string();

        if (extension.empty())
        {
            // No extension provided, try common extensions
            std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
            for (const auto &ext : extensions)
            {
                possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + requested +
                                        ext);
                possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem + ext);
            }
        }
        else
        {
            // Extension provided, use as-is
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + requested);
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem +
                                    extension);
        }

        bool loaded = false;
        for (const auto &resourcePath : possiblePaths)
        {
            if (std::ifstream(resourcePath).good())
            {
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Auto-loading candidate: %s",
                         resourcePath.c_str());
                if (m_models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                {
                    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Auto-loaded model '%s' from %s",
                             (stem.empty() ? requested : stem).c_str(), resourcePath.c_str());
                    loaded = true;
                    break;
                }
            }
        }

        if (!loaded)
        {
            TraceLog(LOG_WARNING,
                     "Game::LoadEditorMap() - Failed to auto-load model referenced by map: %s",
                     requested.c_str());
        }
    }

    // Refresh available list (some models may have been loaded)
    available = m_models->GetAvailableModels();
    for (const auto &object : m_gameMap.objects)
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            // Ensure the model exists in ModelLoader; try fallbacks (stem, auto-load) if missing
            std::string requested = object.modelName;
            auto available = m_models->GetAvailableModels();
            bool exists =
                (std::find(available.begin(), available.end(), requested) != available.end());
            std::string candidateName = requested;

            if (!exists)
            {
                // Try filename stem (strip extension) — many configs use filenames while
                // ModelLoader stores names without extensions
                std::string stem = std::filesystem::path(requested).stem().string();
                if (!stem.empty() &&
                    std::find(available.begin(), available.end(), stem) != available.end())
                {
                    candidateName = stem;
                    exists = true;
                }
                else
                {
                    // Attempt to auto-load from resources using robust path resolution
                    std::vector<std::string> possiblePaths;
                    std::string extension = std::filesystem::path(requested).extension().string();

                    if (extension.empty())
                    {
                        // No extension provided, try common extensions
                        std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
                        for (const auto &ext : extensions)
                        {
                            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                    requested + ext);
                            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                    stem + ext);
                        }
                    }
                    else
                    {
                        // Extension provided, use as-is
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                requested);
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                stem + extension);
                    }

                    bool loaded = false;
                    for (const auto &resourcePath : possiblePaths)
                    {
                        if (std::ifstream(resourcePath).good())
                        {
                            TraceLog(LOG_INFO,
                                     "Game::LoadEditorMap() - Attempting to load model file for "
                                     "instance: %s from %s",
                                     requested.c_str(), resourcePath.c_str());

                            if (!stem.empty() &&
                                m_models->LoadSingleModel(stem, resourcePath, true))
                            {
                                candidateName = stem;
                                exists = true;
                                loaded = true;
                                TraceLog(
                                    LOG_INFO,
                                    "Game::LoadEditorMap() - Auto-loaded model as '%s' from %s",
                                    stem.c_str(), resourcePath.c_str());
                                break;
                            }
                            else if (m_models->LoadSingleModel(requested, resourcePath, true))
                            {
                                candidateName = requested;
                                exists = true;
                                loaded = true;
                                TraceLog(
                                    LOG_INFO,
                                    "Game::LoadEditorMap() - Auto-loaded model as '%s' from %s",
                                    requested.c_str(), resourcePath.c_str());
                                break;
                            }
                        }
                    }

                    if (!loaded)
                    {
                        TraceLog(LOG_WARNING,
                                 "Game::LoadEditorMap() - Could not auto-load model file for %s. "
                                 "Tried paths:",
                                 requested.c_str());
                        for (const auto &path : possiblePaths)
                        {
                            TraceLog(LOG_WARNING, "  - %s", path.c_str());
                        }
                    }
                }
            }

            if (!exists)
            {
                TraceLog(LOG_WARNING,
                         "Game::LoadEditorMap() - Model '%s' not available in ModelLoader; "
                         "skipping instance for object '%s'",
                         requested.c_str(), object.name.c_str());
                continue;
            }

            ModelInstanceConfig cfg;
            cfg.position = object.position;
            cfg.rotation = object.rotation;
            // Use uniform scale from X component, but handle Vector3 scale properly
            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                            ? object.scale.x
                            : 1.0f; // Use X as uniform scale
            cfg.color = object.color;
            cfg.spawn = true;

            bool added = m_models->AddInstanceEx(candidateName, cfg);
            if (!added)
            {
                TraceLog(LOG_WARNING,
                         "Game::LoadEditorMap() - Failed to add instance for model '%s' (object "
                         "'%s') even after load attempts",
                         candidateName.c_str(), object.name.c_str());
            }
            else
            {
                TraceLog(
                    LOG_INFO,
                    "Game::LoadEditorMap() - Added instance for model '%s' at (%.2f, %.2f, %.2f)",
                    candidateName.c_str(), object.position.x, object.position.y, object.position.z);
            }
        }
        else if (object.type == MapObjectType::LIGHT)
        {
            // LIGHT objects are handled separately - no model instances needed
            TraceLog(
                LOG_INFO,
                "Game::LoadEditorMap() - Skipping LIGHT object '%s' for model instance creation",
                object.name.c_str());
        }
    }
}

void Game::RenderEditorMap()
{
    // Render the loaded map objects
    TraceLog(LOG_INFO, "Game::RenderEditorMap() - Rendering %d map objects",
             m_gameMap.objects.size());
    for (const auto &object : m_gameMap.objects)
    {
        // Log object details
        TraceLog(
            LOG_INFO,
            "Game::RenderEditorMap() - Rendering object %s, type %d, position (%.2f, %.2f, %.2f)",
            object.name.c_str(), static_cast<int>(object.type), object.position.x,
            object.position.y, object.position.z);

        // Render based on object type
        switch (object.type)
        {
        case MapObjectType::CUBE:
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing CUBE");
            DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
            break;

        case MapObjectType::SPHERE:
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing SPHERE");
            DrawSphere(object.position, object.radius, object.color);
            break;

        case MapObjectType::CYLINDER:
            // Draw cylinder using multiple spheres for approximation
            // For better cylinder rendering, you might want to use a 3D model
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing CYLINDER");
            DrawSphere(object.position, object.radius, object.color);
            DrawSphere(
                Vector3{object.position.x, object.position.y + object.height, object.position.z},
                object.radius, object.color);
            break;

        case MapObjectType::PLANE:
            // Draw plane as a thin cube
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing PLANE");
            DrawCube(object.position, object.size.x, 0.1f, object.size.y, object.color);
            break;

        case MapObjectType::MODEL:
            // For model objects, try to load and render the actual model
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - MODEL object %s, modelName: %s",
                     object.name.c_str(), object.modelName.c_str());
            if (!object.modelName.empty())
            {
                try
                {
                    if (auto modelOpt = m_models->GetModelByName(object.modelName))
                    {
                        Model &model = modelOpt->get();
                        if (model.meshCount > 0)
                        {
                            TraceLog(LOG_INFO,
                                     "Game::RenderEditorMap() - Model %s found with %d meshes",
                                     object.modelName.c_str(), model.meshCount);
                            // Draw the model with uniform scale
                            DrawModel(model, object.position, object.scale.x, object.color);
                        }
                    }
                    else
                    {
                        TraceLog(LOG_ERROR,
                                 "Game::RenderEditorMap() - Model %s not found or has no meshes!",
                                 object.modelName.c_str());
                        // Fallback to cube if model not found
                        TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing fallback cube for %s",
                                 object.name.c_str());
                        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z,
                                 object.color);
                    }
                }
                catch (const std::exception &e)
                {
                    TraceLog(LOG_ERROR,
                             "Game::RenderEditorMap() - Exception while retrieving model %s: %s",
                             object.modelName.c_str(), e.what());
                    // Fallback to cube if model loading fails
                    TraceLog(
                        LOG_INFO,
                        "Game::RenderEditorMap() - Drawing fallback cube for %s due to exception",
                        object.name.c_str());
                    DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z,
                             object.color);
                }
            }
            else
            {
                TraceLog(
                    LOG_WARNING,
                    "Game::RenderEditorMap() - No modelName for MODEL object %s, drawing as cube",
                    object.name.c_str());
                // No model name specified, draw as cube
                TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing cube for %s (no modelName)",
                         object.name.c_str());
                DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z,
                         object.color);
            }
            break;

        case MapObjectType::LIGHT:
            // LIGHT objects are not rendered as 3D models - they are lighting objects
            TraceLog(LOG_INFO,
                     "Game::RenderEditorMap() - Skipping LIGHT object %s (no 3D rendering needed)",
                     object.name.c_str());
            break;

        default:
            // Unknown type, draw as cube
            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Drawing default cube for unknown type %d",
                     static_cast<int>(object.type));
            DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
            break;
        }
    }
}

void Game::DumpMapDiagnostics() const
{
    TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() - Map objects: %d", m_gameMap.objects.size());

    for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
    {
        const auto &o = m_gameMap.objects[i];
        TraceLog(LOG_INFO,
                 "Game::DumpMapDiagnostics() - Object %d: name='%s' type=%d modelName='%s' "
                 "pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f)",
                 i, o.name.c_str(), static_cast<int>(o.type), o.modelName.c_str(), o.position.x,
                 o.position.y, o.position.z, o.scale.x, o.scale.y, o.scale.z);
    }

    // If MapLoader preloaded models into the map, list them
    if (!m_gameMap.loadedModels.empty())
    {
        TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() - GameMap.loadedModels contains %d entries",
                 m_gameMap.loadedModels.size());
        for (const auto &p : m_gameMap.loadedModels)
        {
            TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() -   loadedModel key: %s (meshCount: %d)",
                     p.first.c_str(), p.second.meshCount);
        }
    }
    else
    {
        TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() - GameMap.loadedModels is empty");
    }

    // List ModelLoader's available models
    auto available = m_models->GetAvailableModels();
    TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() - ModelLoader available models: %d",
             available.size());
    for (const auto &name : available)
    {
        TraceLog(LOG_INFO, "Game::DumpMapDiagnostics() -   %s", name.c_str());
    }
}

void Game::SaveGameState()
{
    TraceLog(LOG_INFO, "Game::SaveGameState() - Saving current game state...");

    // Save the current map path
    m_savedMapPath = m_currentMapPath;
    TraceLog(LOG_INFO, "Game::SaveGameState() - Saved map path: %s", m_savedMapPath.c_str());

    // Save player's current position and state
    m_savedPlayerPosition = m_player->GetPlayerPosition();
    m_savedPlayerVelocity = m_player->GetPhysics().GetVelocity();
    TraceLog(LOG_INFO, "Game::SaveGameState() - Saved player position: (%.2f, %.2f, %.2f)",
             m_savedPlayerPosition.x, m_savedPlayerPosition.y, m_savedPlayerPosition.z);

    // Save game timer/state if available
    // Note: Add any additional game state variables here as needed

    // Enable resume button in menu
    m_menu->SetResumeButtonOn(true);

    TraceLog(LOG_INFO, "Game::SaveGameState() - Game state saved successfully");
}

// Test accessor methods - public for testing purposes (defined in header)

GameMap &Game::GetGameMap() { return m_gameMap; }

bool Game::IsInitialized() const { return m_isGameInitialized; }
