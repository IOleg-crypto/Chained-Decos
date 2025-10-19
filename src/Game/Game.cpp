#include "Game.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/GroundColliderFactory.h"
#include "Engine/Engine.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Model/Model.h"
#include "Game/Menu/Menu.h"
#include "Game/MapEditor/MapFileManager/JsonMapFileManager.h"
#include "Game/Map/MapLoader.h"
#include "Engine/Render/RenderManager.h"
#include "imgui.h"
#include <unordered_set>
#include <fstream>

// Game constants
namespace GameConstants {
    constexpr float DEFAULT_PLATFORM_HEIGHT = 1.0f;
    [[maybe_unused]] constexpr float DEFAULT_PLATFORM_SPACING = 8.0f;
    [[maybe_unused]] constexpr int MAX_MAP_OBJECTS = 1000;
    constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 2.0f;
}

Game::Game(Engine *engine) : m_showMenu(true), m_isGameInitialized(false), m_isDebugInfo(true)
{
    m_engine = engine;
    TraceLog(LOG_INFO, "Game class initialized.");
}

/**
 * @brief Cleanup function to properly release resources
 * Called during game shutdown to ensure clean resource management
 */
void Game::Cleanup()
{
    TraceLog(LOG_INFO, "Game::Cleanup() - Cleaning up game resources...");

    // Clear collision system
    if (!m_collisionManager.GetColliders().empty())
    {
        m_collisionManager.ClearColliders();
        TraceLog(LOG_INFO, "Game::Cleanup() - Collision system cleared");
    }

    // Reset player state
    m_player.SetPlayerPosition({0.0f, 0.0f, 0.0f});
    m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Clear any loaded maps
    if (!m_gameMap.objects.empty())
    {
        m_gameMap.Cleanup();
        TraceLog(LOG_INFO, "Game::Cleanup() - Editor map cleared");
    }

    // Reset game state
    m_showMenu = true;
    m_isGameInitialized = false;
    m_menu.SetGameInProgress(false); // Clear game state when cleaning up

    TraceLog(LOG_INFO, "Game::Cleanup() - Game resources cleaned up successfully");
}

Game::~Game()
{
    TraceLog(LOG_INFO, "Game class destructor called.");
    // Note: Cleanup() should be called explicitly before destruction
    // as destructors should not throw exceptions
}

void Game::Init()
{
    TraceLog(LOG_INFO, "Game::Init() - Initializing game components...");

    // Initialize menu with engine reference (can be null for testing)
    m_menu.Initialize(m_engine);

    // Kernel boot and service registration
    Kernel &kernel = Kernel::GetInstance();
    kernel.Initialize();

    // Only register engine-dependent services if engine is available
    if (m_engine)
    {
        kernel.RegisterService<InputService>(Kernel::ServiceType::Input, std::make_shared<InputService>(&m_engine->GetInputManager()));
    }
    else
    {
        TraceLog(LOG_WARNING, "Game::Init() - No engine provided, skipping engine-dependent services");
    }

    kernel.RegisterService<ModelsService>(Kernel::ServiceType::Models, std::make_shared<ModelsService>(&m_models));
    kernel.RegisterService<WorldService>(Kernel::ServiceType::World, std::make_shared<WorldService>(&m_world));
    // CollisionManager and WorldManager will be registered after creation/initialization

    // Models will be loaded selectively when a map is selected
    // LoadGameModels(); // Commented out - replaced with selective loading
    InitPlayer();
    InitInput();

    m_isGameInitialized = true;
    TraceLog(LOG_INFO, "Game::Init() - Game components initialized.");
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
    }

    // Update kernel services each frame
    Kernel::GetInstance().Update(GetFrameTime());

    // Handle console input (works in both menu and gameplay)
    if (IsKeyPressed(KEY_GRAVE)) // ~ key
    {
        m_menu.ToggleConsole();
    }
    
    // Console input is handled internally by the menu


    // Only process other input if console is not open and engine is available
    if (!m_menu.IsConsoleOpen() && m_engine)
    {
        m_engine->GetInputManager().ProcessInput();
    }

    if (m_showMenu)
    {
        HandleMenuActions();
    }
    else
    {
        // Only update game logic if console is not open
        if (!m_menu.IsConsoleOpen())
        {
            // Debug: Check collision system before updating player
            size_t colliderCount = m_collisionManager.GetColliders().size();
            TraceLog(LOG_INFO, "Game::Update() - Collision system has %d colliders", colliderCount);
    
            UpdatePlayerLogic();
            UpdatePhysicsLogic();
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
        m_engine->GetRenderManager()->RenderMenu(m_menu);
    }
    else
    {
        // Add debug info before rendering
        TraceLog(LOG_INFO, "Game::Render() - Rendering game world, player position: (%.2f, %.2f, %.2f)",
                 m_player.GetPlayerPosition().x, m_player.GetPlayerPosition().y, m_player.GetPlayerPosition().z);

        // Debug: Check if models are loaded
        auto availableModels = m_models.GetAvailableModels();
        TraceLog(LOG_INFO, "Game::Render() - Available models: %d", availableModels.size());
        for (const auto& modelName : availableModels)
        {
            TraceLog(LOG_INFO, "Game::Render() -   Model: %s", modelName.c_str());
        }

        RenderGameWorld();
        RenderGameUI();

        // Add debug info after rendering
        if (!m_gameMap.objects.empty())
        {
            TraceLog(LOG_INFO, "Game::Render() - Rendered %d map objects", m_gameMap.objects.size());
        }
    }

    if (m_engine->IsDebugInfoVisible() && !m_showMenu)
    {
        m_engine->GetRenderManager()->RenderDebugInfo(m_player, m_models, m_collisionManager);
    }

    // Console is rendered internally by the menu

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
                                                       // Set game as in progress when going to menu from game
                                                       if (!m_showMenu)
                                                       {
                                                           m_menu.SetGameInProgress(true);
                                                       }
                                                       m_showMenu = true;
                                                       EnableCursor();
                                                   });

    m_engine->GetInputManager().RegisterAction(KEY_ESCAPE,
                                                   [this]
                                                   {
                                                       if (!m_showMenu)
                                                       {
                                                           m_menu.ResetAction();
                                                           // Set game as in progress when going to menu from game
                                                           m_menu.SetGameInProgress(true);
                                                           ToggleMenu();
                                                           EnableCursor();
                                                       }
                                                   });
    TraceLog(LOG_INFO, "Game::InitInput() - Game input bindings configured.");
}

void Game::InitCollisions()
{
    TraceLog(LOG_INFO, "Game::InitCollisions() - Initializing collision system...");

    // Clear existing colliders if any
    size_t previousColliderCount = m_collisionManager.GetColliders().size();
    if (previousColliderCount > 0)
    {
        TraceLog(LOG_INFO, "Game::InitCollisions() - Clearing %zu existing colliders", previousColliderCount);
        m_collisionManager.ClearColliders();
    }

    // Only create artificial ground if we don't have a custom map with its own ground
    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO, "Game::InitCollisions() - No custom map loaded, creating default ground");
        Collision groundPlane = GroundColliderFactory::CreateDefaultGameGround();
        m_collisionManager.AddCollider(std::move(groundPlane));
    }
    else
    {
        TraceLog(LOG_INFO, "Game::InitCollisions() - Custom map loaded, using map's ground objects");
    }

    // Create parkour test map based on menu selection
    MenuAction action = m_menu.GetAction();
    switch(action)
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
    m_collisionManager.Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(Kernel::ServiceType::Collision, std::make_shared<CollisionService>(&m_collisionManager));

    // Load model collisions only for models that are actually loaded and required for this map
    auto availableModels = m_models.GetAvailableModels();
    TraceLog(LOG_INFO, "Game::InitCollisions() - Available models for collision generation: %d", availableModels.size());
    for (const auto& modelName : availableModels)
    {
        TraceLog(LOG_INFO, "Game::InitCollisions() - Model available: %s", modelName.c_str());
    }

    try
    {
        m_collisionManager.CreateAutoCollisionsFromModelsSelective(m_models, availableModels);
        TraceLog(LOG_INFO, "Game::InitCollisions() - Model collisions created successfully");
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Game::InitCollisions() - Failed to create model collisions: %s", e.what());
        TraceLog(LOG_WARNING, "Game::InitCollisions() - Continuing without model collisions");
    }

    // Reinitialize after adding all model colliders
    m_collisionManager.Initialize();

    // Initialize player collision
    auto& playerCollision = m_player.GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(LOG_INFO, "Game::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager.GetColliders().size());
}

void Game::InitCollisionsWithModels(const std::vector<std::string>& requiredModels)
{
    TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Initializing collision system with %d required models...", requiredModels.size());

    // Clear existing colliders if any
    size_t previousColliderCount = m_collisionManager.GetColliders().size();
    if (previousColliderCount > 0)
    {
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Clearing %zu existing colliders", previousColliderCount);
        m_collisionManager.ClearColliders();
    }
    else
    {
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Custom map loaded, using map's ground objects");
    }

    // Create parkour test map based on menu selection
    MenuAction action = m_menu.GetAction();
    switch(action)
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
    m_collisionManager.Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(Kernel::ServiceType::Collision, std::make_shared<CollisionService>(&m_collisionManager));

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Required models for collision generation: %d", requiredModels.size());
    for (const auto& modelName : requiredModels)
    {
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Model required: %s", modelName.c_str());
    }

    try
    {
        m_collisionManager.CreateAutoCollisionsFromModelsSelective(m_models, requiredModels);
        TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Model collisions created successfully");
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Game::InitCollisionsWithModels() - Failed to create model collisions: %s", e.what());
        TraceLog(LOG_WARNING, "Game::InitCollisionsWithModels() - Continuing without model collisions");
    }
    catch (...)
    {
        TraceLog(LOG_ERROR, "Game::InitCollisionsWithModels() - Unknown error occurred during model collision creation");
        TraceLog(LOG_WARNING, "Game::InitCollisionsWithModels() - Continuing without model collisions");
    }

    // Reinitialize after adding all model colliders
    m_collisionManager.Initialize();

    // Initialize player collision
    auto& playerCollision = m_player.GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(LOG_INFO, "Game::InitCollisionsWithModels() - Collision system initialized with %zu colliders.",
              m_collisionManager.GetColliders().size());
}

void Game::InitPlayer()
{
    TraceLog(LOG_INFO, "Game::InitPlayer() - Initializing player...");

    // Set initial position on the first platform (mix of ground and floating platforms)
    Vector3 safePosition = {0.0f, GameConstants::PLAYER_SAFE_SPAWN_HEIGHT, 0.0f};
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting initial safe position: (%.2f, %.2f, %.2f)",
             safePosition.x, safePosition.y, safePosition.z);
    m_player.SetPlayerPosition(safePosition);

    // Setup collision and physics
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting up collision manager for player...");
    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);

    TraceLog(LOG_INFO, "Game::InitPlayer() - Updating player collision box...");
    m_player.UpdatePlayerBox();

    TraceLog(LOG_INFO, "Game::InitPlayer() - Updating player collision...");
    m_player.UpdatePlayerCollision();

    // Allow physics to determine grounded state; start ungrounded so gravity applies
    TraceLog(LOG_INFO, "Game::InitPlayer() - Setting initial physics state...");
    m_player.GetPhysics().SetGroundLevel(false);
    m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Load player model with improved error handling and fallback
    TraceLog(LOG_INFO, "Game::InitPlayer() - Loading player model...");
    try
    {
        // First try to load the player model
        Model* playerModel = &m_models.GetModelByName("player");
        if (playerModel && playerModel->meshCount > 0)
        {
            m_player.SetPlayerModel(playerModel);
            TraceLog(LOG_INFO, "Game::InitPlayer() - Player model loaded successfully.");
        }
        else
        {
            TraceLog(LOG_ERROR, "Game::InitPlayer() - Player model is invalid or has no meshes");

            // Test if other models work - try loading plane.glb as a fallback test
            TraceLog(LOG_INFO, "Game::InitPlayer() - Testing if other models can be loaded...");
            Model* testModel = &m_models.GetModelByName("plane");
            if (testModel && testModel->meshCount > 0)
            {
                TraceLog(LOG_INFO, "Game::InitPlayer() - Other models load successfully (plane.glb works)");
                TraceLog(LOG_INFO, "Game::InitPlayer() - Issue is specific to player.glb file");
            }
            else
            {
                TraceLog(LOG_ERROR, "Game::InitPlayer() - Other models also fail to load");
                TraceLog(LOG_INFO, "Game::InitPlayer() - Issue may be with GLB format or raylib loader");
            }

            // Create a simple fallback player model using basic shapes
            TraceLog(LOG_INFO, "Game::InitPlayer() - Creating fallback player model using basic shapes...");
            TraceLog(LOG_INFO, "Game::InitPlayer() - Using default player rendering (no 3D model)");
            TraceLog(LOG_WARNING, "Game::InitPlayer() - Player will use default rendering");
        }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Game::InitPlayer() - Failed to load player model: %s", e.what());
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player will use default rendering");
    }

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
                 safePosition.x, safePosition.y, safePosition.z);

    // Additional safety check - ensure player is properly positioned
    Vector3 currentPos = m_player.GetPlayerPosition();
    TraceLog(LOG_INFO, "Game::InitPlayer() - Player current position: (%.2f, %.2f, %.2f)",
                 currentPos.x, currentPos.y, currentPos.z);

    // Validate player position is safe (above ground but not too high)
    if (currentPos.y < 0.0f)
    {
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player position below ground level, adjusting");
        m_player.SetPlayerPosition({currentPos.x, GameConstants::PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    else if (currentPos.y > 50.0f)
    {
        TraceLog(LOG_WARNING, "Game::InitPlayer() - Player position too high, adjusting");
        m_player.SetPlayerPosition({currentPos.x, GameConstants::PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }

    // Check if map has PlayerStart objects and adjust position accordingly
    TraceLog(LOG_INFO, "Game::InitPlayer() - Checking for PlayerStart objects in map...");
    if (!m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO, "Game::InitPlayer() - Map has %d objects, searching for PlayerStart...", m_gameMap.objects.size());
        for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
        {
            const auto& obj = m_gameMap.objects[i];
            TraceLog(LOG_INFO, "Game::InitPlayer() - Checking object %d: %s (type: %d)", i, obj.name.c_str(), static_cast<int>(obj.type));

            if ((obj.type == MapObjectType::MODEL || obj.type == MapObjectType::LIGHT) && obj.name.find("player_start") != std::string::npos)
            {
                TraceLog(LOG_INFO, "Game::InitPlayer() - Found PlayerStart object at (%.2f, %.2f, %.2f)",
                         obj.position.x, obj.position.y, obj.position.z);
                m_player.SetPlayerPosition(obj.position);
                TraceLog(LOG_INFO, "Game::InitPlayer() - Player position updated to PlayerStart location");
                break;
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "Game::InitPlayer() - No map objects found, using default position");
    }

    // Final position verification
    Vector3 finalPos = m_player.GetPlayerPosition();
    TraceLog(LOG_INFO, "Game::InitPlayer() - Final player position: (%.2f, %.2f, %.2f)",
             finalPos.x, finalPos.y, finalPos.z);

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialization complete");
}

void Game::LoadGameModels()
{
    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading game models from resources directory...");
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(50);
    m_models.EnableLOD(true);
    m_models.SetSelectiveMode(false);

    try
    {
        // Use the new MapLoader to scan for models in the resources directory
        MapLoader mapLoader;
        std::string resourcesDir = "./resources";
        auto models = mapLoader.LoadModelsFromDirectory(resourcesDir);

        if (!models.empty())
        {
            TraceLog(LOG_INFO, "Game::LoadGameModels() - Found %d models in resources directory", models.size());

            // Load each model found in the directory
            for (const auto& modelInfo : models)
            {
                try
                {
                    std::string modelPath = modelInfo.path;
                    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading model: %s from %s",
                             modelInfo.name.c_str(), modelPath.c_str());

                    // Load the model using the existing model loading system
                    m_models.LoadSingleModel(modelInfo.name, modelPath, true);
                }
                catch (const std::exception& modelException)
                {
                    TraceLog(LOG_WARNING, "Game::LoadGameModels() - Failed to load model %s: %s",
                             modelInfo.name.c_str(), modelException.what());
                }
            }

            m_models.PrintStatistics();
            TraceLog(LOG_INFO, "Game::LoadGameModels() - Models loaded successfully.");

            // Validate that we have essential models
            auto availableModels = m_models.GetAvailableModels();
            bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player_low") != availableModels.end();

            if (!hasPlayerModel)
            {
                TraceLog(LOG_WARNING, "Game::LoadGameModels() - Player model not found, player may not render correctly");
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Game::LoadGameModels() - No models found in resources directory");
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Game::LoadGameModels() - Failed to load models: %s", e.what());
        TraceLog(LOG_ERROR, "Game::LoadGameModels() - Game may not function correctly without models");
    }
}

void Game::LoadGameModelsSelective(const std::vector<std::string>& modelNames)
{
    TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Loading selective models: %d models", modelNames.size());
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(50);
    m_models.EnableLOD(false);
    m_models.SetSelectiveMode(true);

    try
    {
        // Use the new MapLoader to scan for models in the resources directory
        MapLoader mapLoader;
        std::string resourcesDir = "./resources";
        auto allModels = mapLoader.LoadModelsFromDirectory(resourcesDir);

        if (!allModels.empty())
        {
            TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Found %d models in resources directory", allModels.size());

            // Load only the models that are in the required list
            for (const auto& modelInfo : allModels)
            {
                // Check if this model is in the required list
                if (std::find(modelNames.begin(), modelNames.end(), modelInfo.name) != modelNames.end())
                {
                    try
                    {
                        std::string modelPath = modelInfo.path;
                        TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Loading required model: %s from %s",
                                 modelInfo.name.c_str(), modelPath.c_str());

                        // Load the model using the existing model loading system
                        m_models.LoadSingleModel(modelInfo.name, modelPath, true);
                    }
                    catch (const std::exception& modelException)
                    {
                        TraceLog(LOG_WARNING, "Game::LoadGameModelsSelective() - Failed to load model %s: %s",
                                 modelInfo.name.c_str(), modelException.what());
                    }
                }
            }

            m_models.PrintStatistics();
            TraceLog(LOG_INFO, "Game::LoadGameModelsSelective() - Selective models loaded successfully.");

            // Validate that we have essential models
            auto availableModels = m_models.GetAvailableModels();
            bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") != availableModels.end();

            if (!hasPlayerModel)
            {
                TraceLog(LOG_WARNING, "Game::LoadGameModelsSelective() - Player model not found, player may not render correctly");
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Game::LoadGameModelsSelective() - No models found in resources directory");
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Game::LoadGameModelsSelective() - Failed to load selective models: %s", e.what());
        TraceLog(LOG_ERROR, "Game::LoadGameModelsSelective() - Game may not function correctly without models");
    }
}

///
/// @brief Maps object types to appropriate model names for selective loading
/// @param objectType The MapObjectType enum value
/// @return Model name if mapping exists, empty string otherwise
///
std::string Game::GetModelNameForObjectType(int objectType, const std::string& modelName)
{
    // Handle MODEL type objects (type 4) and incorrectly exported MODEL objects (type 5)
    // The map editor seems to be exporting MODEL objects as type 5 instead of type 4
    if (objectType == 4 || objectType == 5) // MapObjectType::MODEL or incorrectly exported as LIGHT
    {
        // For MODEL type objects, return the actual model name if provided
        if (!modelName.empty())
        {
            return modelName;
        }
        // For backward compatibility, return empty string if no model name provided
        return "";
    }

    // For non-MODEL types, return empty string as they don't require 3D models
    return "";
}

std::vector<std::string> Game::GetModelsRequiredForMap(const std::string& mapIdentifier)
{
    std::vector<std::string> requiredModels;

    // Always include the player model as it's essential for gameplay
    requiredModels.emplace_back("player");

    // Convert map name to full path if needed
    std::string mapPath = mapIdentifier;
    if (mapPath.substr(mapPath.find_last_of('.') + 1) != "json")
    {
        // If it's not a path ending in .json, assume it's a map name and construct the path
        mapPath = "./src/Game/Resource/maps/" + mapIdentifier;
        if (mapIdentifier.find(".json") == std::string::npos)
        {
            mapPath += ".json";
        }
    }

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    if (extension == "json")
    {
        TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Analyzing JSON map for model requirements: %s", mapPath.c_str());

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
                        for (const auto& object : j["objects"])
                        {
                            // Check if this object references a model by modelName
                            if (object.contains("modelName") && object["modelName"].is_string())
                            {
                                std::string modelName = object["modelName"].get<std::string>();
                                if (!modelName.empty())
                                {
                                    // Check if this model is not already in the list
                                    if (std::find(requiredModels.begin(), requiredModels.end(), modelName) == requiredModels.end())
                                    {
                                        requiredModels.push_back(modelName);
                                        TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Found model requirement: %s", modelName.c_str());
                                    }
                                }
                            }
                            // Also check object type and map to appropriate models for MODEL type objects
                            else if (object.contains("type") && object["type"].is_number_integer())
                            {
                                int objectType = object["type"].get<int>();
                                std::string objectModelName = "";
                                if (object.contains("modelName") && object["modelName"].is_string())
                                {
                                    objectModelName = object["modelName"].get<std::string>();
                                }

                                std::string modelName = GetModelNameForObjectType(objectType, objectModelName);

                                if (!modelName.empty() && std::find(requiredModels.begin(), requiredModels.end(), modelName) == requiredModels.end())
                                {
                                    requiredModels.push_back(modelName);
                                    TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Mapped type %d to model: %s", objectType, modelName.c_str());
                                }
                            }
                        }
                    }
                }
                else if (arrayStart != std::string::npos)
                {
                    // This is the game format (direct array) - parse manually
                    TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Detected game format, parsing manually");

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
                            std::string objectJson = content.substr(objectStart, objectEnd - objectStart + 1);

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
                                        std::string modelName = objectJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                                        if (!modelName.empty())
                                        {
                                            // Check if this model is not already in the list
                                            if (std::find(requiredModels.begin(), requiredModels.end(), modelName) == requiredModels.end())
                                            {
                                                requiredModels.push_back(modelName);
                                                TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Found model requirement: %s", modelName.c_str());
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
                                            std::string modelName = objectJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                                            if (!modelName.empty())
                                            {
                                                // Check if this model is not already in the list
                                                if (std::find(requiredModels.begin(), requiredModels.end(), modelName) == requiredModels.end())
                                                {
                                                    requiredModels.push_back(modelName);
                                                    TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Found model requirement in game format: %s", modelName.c_str());
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
                    TraceLog(LOG_WARNING, "Game::GetModelsRequiredForMap() - No valid JSON structure found in map file");
                }
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_WARNING, "Game::GetModelsRequiredForMap() - Error parsing map JSON: %s", e.what());
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Game::GetModelsRequiredForMap() - Could not open map file: %s", mapPath.c_str());
        }
    }
    else
    {
        TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Non-JSON map format, using default model set");
        // For non-JSON maps, include common models that might be needed
        requiredModels.emplace_back("arena");
    }

    TraceLog(LOG_INFO, "Game::GetModelsRequiredForMap() - Total models required: %d", requiredModels.size());
    return requiredModels;
}

void Game::UpdatePlayerLogic()
{
    if (!m_engine)
    {
        // Skip player logic if no engine is available (for testing)
        m_player.Update(m_collisionManager);
        return;
    }

    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        m_engine->GetRenderManager()->ShowMetersPlayer(m_player);
        return;
    }

    m_player.Update(m_collisionManager);

    m_engine->GetRenderManager()->ShowMetersPlayer(m_player);
}

/**
 * @brief Updates physics-related game logic
 *
 * Ensures collision system is properly initialized and handles
 * edge cases where collision data might be missing.
 */
void Game::UpdatePhysicsLogic()
{
    const auto& colliders = m_collisionManager.GetColliders();

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
                // Collision plane = GroundColliderFactory::CreateDefaultGameGround();
                // m_collisionManager.AddCollider(std::move(plane));
                TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - Created emergency ground plane.");
            }
            else
            {
                TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - No colliders but custom map loaded, using map objects for collision.");
            }
        }
        catch (const std::exception& e)
        {
            TraceLog(LOG_ERROR, "Game::UpdatePhysicsLogic() - Failed to create emergency ground plane: %s", e.what());
        }
    }
    else if (colliders.size() < 2) // Only ground plane exists
    {
        static bool infoShown = false;
        if (!infoShown)
        {
            TraceLog(LOG_INFO, "Game::UpdatePhysicsLogic() - Only ground plane available, no gameplay platforms");
            infoShown = true;
        }
    }
}


void Game::HandleMenuActions()
{
    MenuAction action = m_menu.ConsumeAction(); // Use ConsumeAction instead of GetAction
    switch (action)
    {
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting singleplayer...");
        m_menu.SetGameInProgress(true);

        // Initialize player after map is loaded
        try
        {
            InitPlayer();
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized successfully");
        }
        catch (const std::exception& e)
        {
            TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize player: %s", e.what());
            TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Player may not render correctly");
        }

        ToggleMenu();
        m_isGameInitialized = true; // Mark game as initialized
        break;

    case MenuAction::ResumeGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resuming game...");
        m_menu.SetAction(MenuAction::SinglePlayer);
        // Ensure game is properly initialized for resume
        if (!m_isGameInitialized)
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing game for resume...");

            // Load models for the current map (test.json for singleplayer)
            std::string testMapPath = "./src/Game/Resource/test.json";
            std::vector<std::string> requiredModels = GetModelsRequiredForMap(testMapPath);
            LoadGameModelsSelective(requiredModels);

            // Initialize basic collision system first
            try
            {
                InitCollisionsWithModels(requiredModels);
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Collision system initialized for singleplayer");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize basic collision system for singleplayer: %s", e.what());
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without collision system");
                return;
            }

            // Load the test map
            try
            {
                LoadEditorMap(testMapPath);
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resume map loaded successfully");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load resume map: %s", e.what());
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot resume without map");
                return;
            }

            // Initialize player after map is loaded
            try
            {
                InitPlayer();
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized for resume");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize player for resume: %s", e.what());
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Player may not render correctly");
            }
        
        }
        else
        {
            // Game is already initialized, just ensure collision system is ready
            if (m_collisionManager.GetColliders().empty())
            {
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - No colliders found, reinitializing...");
                // Recalculate required models for the current map
                std::string testMapPath = "./src/Game/Resource/test.json";
                std::vector<std::string> requiredModels = GetModelsRequiredForMap(testMapPath);

                // Reinitialize collision system safely
                try
                {
                    // Clear existing colliders
                    m_collisionManager.ClearColliders();

                    // Create ground collision first (only if no custom map)
                    if (m_gameMap.objects.empty())
                    {
                        Collision groundPlane = GroundColliderFactory::CreateDefaultGameGround();
                        m_collisionManager.AddCollider(std::move(groundPlane));
                    }

                    // Initialize collision manager
                    m_collisionManager.Initialize();

                    // Try to create model collisions, but don't fail if it doesn't work
                    try
                    {
                        m_collisionManager.CreateAutoCollisionsFromModelsSelective(m_models, requiredModels);
                        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resume model collisions created successfully");
                    }
                    catch (const std::exception& modelCollisionException)
                    {
                        TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Resume model collision creation failed: %s", modelCollisionException.what());
                        TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Continuing with basic collision system only");
                    }
                }
                catch (const std::exception& e)
                {
                    TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to reinitialize collision system for resume: %s", e.what());
                }
            }

            // Ensure player is properly positioned and set up
            if (m_player.GetPlayerPosition().x == 0.0f &&
                m_player.GetPlayerPosition().y == 0.0f &&
                m_player.GetPlayerPosition().z == 0.0f)
            {
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player position is origin, resetting to safe position");
                m_player.SetPlayerPosition({0.0f, GameConstants::PLAYER_SAFE_SPAWN_HEIGHT, 0.0f});
            }

            // Re-setup player collision and movement
            m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
            m_player.UpdatePlayerBox();
            m_player.UpdatePlayerCollision();
        }

        // Hide the menu and resume the game
        m_showMenu = false;
        HideCursor();
        m_menu.ResetAction();
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Game resumed successfully");
        // Keep game in progress state when resuming
        break;
    case MenuAction::StartGameWithMap:
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting game with selected map...");
            m_menu.SetGameInProgress(true);
            std::string selectedMapName = m_menu.GetSelectedMapName();
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Selected map: %s", selectedMapName.c_str());

            // Convert map name to full path
            std::string mapPath;
            if (selectedMapName.find('/') != std::string::npos || selectedMapName.find('\\') != std::string::npos)
            {
                // Check if this is already an absolute path (starts with drive letter like D:/)
                if (selectedMapName.length() >= 3 &&
                    isalpha(selectedMapName[0]) &&
                    selectedMapName[1] == ':' &&
                    (selectedMapName[2] == '/' || selectedMapName[2] == '\\'))
                {
                    // Already an absolute path, use as-is
                    mapPath = selectedMapName;
                }
                else
                {
                    // Relative path with separators, prepend current directory
                    mapPath = "./" + selectedMapName;
                }
            }
            else
            {
                // selectedMapName is just a filename, construct full path
                mapPath = "./src/Game/Resource/maps/" + selectedMapName;
                if (selectedMapName.find(".json") == std::string::npos)
                {
                    mapPath += ".json";
                }
            }
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Full map path: %s", mapPath.c_str());

            // Determine which models are required for this map
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Determining required models...");
            std::vector<std::string> requiredModels;
            
            // For parkour map, load parkour models
            if (selectedMapName.find("parkourmap") != std::string::npos)
            {
                requiredModels = {"plane", "player", "arena", "bridge", "stairs", "section_of_walls"};
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Parkour map detected, loading parkour models");
            }
            else if (selectedMapName.find("exported_map1") != std::string::npos)
            {
                requiredModels = {"plane", "player", "stairs_f"};
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Exported map detected, loading exported map models");
            }
            else if (selectedMapName.find("test") != std::string::npos)
            {
                requiredModels = {"plane", "player"};
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Test map detected, loading basic models");
            }
            else
            {
                // Try to get models from map file
                requiredModels = GetModelsRequiredForMap(selectedMapName);
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading models from map file");
            }
            
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Required models:");
            for (const auto& model : requiredModels)
            {
                TraceLog(LOG_INFO, "Game::HandleMenuActions() -   - %s", model.c_str());
            }
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading %d models for map", requiredModels.size());

            // Load only the required models selectively
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading selective models...");
            try
            {
                LoadGameModelsSelective(requiredModels);
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Models loaded successfully");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load models: %s", e.what());
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Continuing with available models");
            }

            // Initialize basic collision system first
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing collision system...");
            try
            {
                InitCollisionsWithModels(requiredModels);
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Collision system initialized");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize basic collision system: %s", e.what());
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without collision system");
                return;
            }

            // Load the selected map
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading selected map...");
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
                        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Detected array format, using LoadModelsMap");
                        m_gameMap = LoadGameMap(mapPath.c_str());
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

                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Map loaded successfully");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load map: %s", e.what());
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Continuing with default map");
                // Load default test map as fallback
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Loading fallback map...");
                try
                {
                    LoadEditorMap("./src/Game/Resource/test.json");
                    TraceLog(LOG_INFO, "Game::HandleMenuActions() - Fallback map loaded successfully");
                }
                catch (const std::exception& e2)
                {
                    TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to load fallback map: %s", e2.what());
                    TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Cannot continue without any map");
                    return;
                }
            }

            // Initialize player after map is loaded
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing player...");
            try
            {
                InitPlayer();
                TraceLog(LOG_INFO, "Game::HandleMenuActions() - Player initialized successfully");
            }
            catch (const std::exception& e)
            {
                TraceLog(LOG_ERROR, "Game::HandleMenuActions() - Failed to initialize player: %s", e.what());
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - Player may not render correctly");
            }
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Game initialization complete");
            m_isGameInitialized = true; // Mark game as initialized

            // Hide menu and start the game
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Hiding menu and starting game...");
            m_showMenu = false;
            HideCursor();

            m_menu.ResetAction();
        }
        break;

    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Exit game requested from menu.");
        // Clear game state when exiting
        m_menu.SetGameInProgress(false);
        m_showMenu = true; // Show menu one last time before exit
        if (m_engine)
        {
            m_engine->RequestExit();
        }
        m_menu.ResetAction();
        break;

    default:
        break;
    }
}

void Game::RenderGameWorld() {
    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "Game::RenderGameWorld() - No engine provided, skipping game world render");
        return;
    }

    m_engine->GetRenderManager()->RenderGame(m_player, m_models, m_collisionManager,
                                              m_engine->IsCollisionDebugVisible());

    // Render editor-created map if available
    if (!m_gameMap.objects.empty())
    {
        RenderEditorMap();
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Creates a platform with collision box at specified position
 * @param position Platform center position in 3D space
 * @param size Platform dimensions (width, height, depth)
 * @param color Platform render color
 * @param collisionType Type of collision detection to use
 *
 * This helper function reduces code duplication in map creation functions
 * and ensures consistent platform creation across all map types.
 */
void Game::CreatePlatform(const Vector3& position, const Vector3& size, Color color, CollisionType collisionType)
{
    DrawCube(position, size.x, size.y, size.z, color);

    Collision collision(position, size);
    collision.SetCollisionType(collisionType);
    m_collisionManager.AddCollider(std::move(collision));
}

/**
 * @brief Calculates dynamic font size based on screen resolution
 * @param baseSize Base font size for 1920p resolution
 * @return Scaled font size clamped to reasonable bounds
 */
float Game::CalculateDynamicFontSize(float baseSize) const
{
    int screenWidth = GetScreenWidth();
    float scaleFactor = static_cast<float>(screenWidth) / 1920.0f;
    float dynamicSize = baseSize * scaleFactor;

    // Clamp to reasonable bounds
    return std::max(18.0f, std::min(48.0f, dynamicSize));
}

void Game::RenderGameUI() const {
    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "Game::RenderGameUI() - No engine provided, skipping game UI render");
        return;
    }

    m_engine->GetRenderManager()->ShowMetersPlayer(m_player);

    static float gameTime = 0.0f;
    gameTime += GetFrameTime();

    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;
    int milliseconds = static_cast<int>((gameTime - static_cast<float>(static_cast<int>(gameTime))) * 1000);

    // Add timer icon using ASCII art timer (works on all systems)
    const char* timerIcon = "[TIMER] ";
    std::string timerText = TextFormat("%s%02d:%02d:%03d", timerIcon, minutes, seconds, milliseconds);

    Vector2 timerPos = {300.0f, 20.0f};

    Font fontToUse = (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
                          ? m_engine->GetRenderManager()->GetFont()
                          : GetFontDefault();

    float fontSize = CalculateDynamicFontSize(24.0f);
    DrawTextEx(fontToUse, timerText.c_str(), timerPos, fontSize, 2.0f, WHITE);
}


void Game::CreateParkourTestMap()
{
    // Advanced test map using Raylib functions directly
    TraceLog(LOG_INFO, "Game::CreateParkourTestMap() - Creating test parkour map");

    // Starting platform - larger for safe landing
    CreatePlatform({0.0f, 0.0f, 0.0f}, {4.0f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 4.0f}, DARKGREEN, CollisionType::AABB_ONLY);

    // First jump platform
    CreatePlatform({8.0f, 0.0f, 2.0f}, {2.0f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 2.0f}, DARKBLUE, CollisionType::AABB_ONLY);

    // Floating challenge platform
    CreatePlatform({14.0f, 4.0f, 1.0f}, {1.5f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 1.5f}, DARKPURPLE, CollisionType::AABB_ONLY);

    // Mid-way platform
    CreatePlatform({20.0f, 1.0f, -1.0f}, {2.5f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 2.5f}, DARKBROWN, CollisionType::AABB_ONLY);

    // High precision platform
    CreatePlatform({26.0f, 6.0f, 0.0f}, {1.2f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 1.2f}, RED, CollisionType::AABB_ONLY);

    // Final platform
    CreatePlatform({32.0f, 2.0f, -2.0f}, {3.0f, GameConstants::DEFAULT_PLATFORM_HEIGHT, 3.0f}, GOLD, CollisionType::AABB_ONLY);

    TraceLog(LOG_INFO, "Game::CreateParkourTestMap() - Test map created successfully");
}

void Game::CreateEasyParkourMap()
{
    // Advanced Easy parkour map using Raylib functions
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Starting area
    DrawCube(startPos, 5.0f, 1.0f, 5.0f, DARKGREEN);
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {5.0f, 1.0f, 5.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Gentle first platforms
    Vector3 plat1 = {10.0f, 0.0f, 4.0f};
    DrawCube(plat1, 3.0f, 1.0f, 3.0f, DARKBLUE);
    Collision c1({10.0f, 0.0f, 4.0f}, {3.0f, 1.0f, 3.0f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Low floating platform
    Vector3 plat2 = {20.0f, 3.0f, 2.0f};
    DrawCube(plat2, 2.5f, 1.0f, 2.5f, DARKPURPLE);
    Collision c2({20.0f, 3.0f, 2.0f}, {2.5f, 1.0f, 2.5f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Ground platform with ramp approach
    Vector3 plat3 = {30.0f, 0.0f, -1.0f};
    DrawCube(plat3, 3.5f, 1.0f, 3.5f, DARKBROWN);
    Collision c3({30.0f, 0.0f, -1.0f}, {3.5f, 1.0f, 3.5f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // Medium height challenge
    Vector3 plat4 = {42.0f, 5.0f, 1.0f};
    DrawCube(plat4, 2.2f, 1.0f, 2.2f, RED);
    Collision c4({42.0f, 5.0f, 1.0f}, {2.2f, 1.0f, 2.2f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Rest platform
    Vector3 plat5 = {52.0f, 1.0f, -2.0f};
    DrawCube(plat5, 3.0f, 1.0f, 3.0f, DARKGRAY);
    Collision c5({52.0f, 1.0f, -2.0f}, {3.0f, 1.0f, 3.0f});
    c5.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c5));

    // Higher challenge platform
    Vector3 plat6 = {62.0f, 7.0f, 0.0f};
    DrawCube(plat6, 2.0f, 1.0f, 2.0f, ORANGE);
    Collision c6({62.0f, 7.0f, 0.0f}, {2.0f, 1.0f, 2.0f});
    c6.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c6));

    // Descent platforms
    Vector3 plat7 = {72.0f, 3.0f, 2.0f};
    DrawCube(plat7, 2.8f, 1.0f, 2.8f, DARKBLUE);
    Collision c7({72.0f, 3.0f, 2.0f}, {2.8f, 1.0f, 2.8f});
    c7.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c7));

    Vector3 plat8 = {82.0f, 1.0f, -1.0f};
    DrawCube(plat8, 2.5f, 1.0f, 2.5f, DARKPURPLE);
    Collision c8({82.0f, 1.0f, -1.0f}, {2.5f, 1.0f, 2.5f});
    c8.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c8));

    // Final platform
    Vector3 endPos = {92.0f, 0.0f, 1.0f};
    DrawCube(endPos, 4.0f, 1.0f, 4.0f, GOLD);
    Collision endPlatform({92.0f, 0.0f, 1.0f}, {4.0f, 1.0f, 4.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateMediumParkourMap()
{
    // Advanced Medium difficulty using Raylib functions
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Large starting area
    DrawCube(startPos, 4.0f, 1.0f, 4.0f, DARKGREEN);
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Challenging platform sequence
    Vector3 plat1 = {12.0f, 0.0f, 5.0f};
    DrawCube(plat1, 2.2f, 1.0f, 2.2f, DARKBLUE);
    Collision c1({12.0f, 0.0f, 5.0f}, {2.2f, 1.0f, 2.2f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Precision jump platform
    Vector3 plat2 = {22.0f, 5.0f, 3.0f};
    DrawCube(plat2, 1.8f, 1.0f, 1.8f, DARKPURPLE);
    Collision c2({22.0f, 5.0f, 3.0f}, {1.8f, 1.0f, 1.8f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Moving platform simulation (static but challenging position)
    Vector3 plat3 = {32.0f, 2.0f, -2.0f};
    DrawCube(plat3, 2.0f, 1.0f, 2.0f, DARKBROWN);
    Collision c3({32.0f, 2.0f, -2.0f}, {2.0f, 1.0f, 2.0f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // High altitude challenge
    Vector3 plat4 = {44.0f, 8.0f, 1.0f};
    DrawCube(plat4, 1.5f, 1.0f, 1.5f, RED);
    Collision c4({44.0f, 8.0f, 1.0f}, {1.5f, 1.0f, 1.5f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Recovery platform
    Vector3 plat5 = {54.0f, 3.0f, -1.0f};
    DrawCube(plat5, 2.5f, 1.0f, 2.5f, DARKGRAY);
    Collision c5({54.0f, 3.0f, -1.0f}, {2.5f, 1.0f, 2.5f});
    c5.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c5));

    // Very high precision platform
    Vector3 plat6 = {66.0f, 10.0f, 2.0f};
    DrawCube(plat6, 1.2f, 1.0f, 1.2f, ORANGE);
    Collision c6({66.0f, 10.0f, 2.0f}, {1.2f, 1.0f, 1.2f});
    c6.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c6));

    // Descent platform 1
    Vector3 plat7 = {76.0f, 6.0f, 0.0f};
    DrawCube(plat7, 2.0f, 1.0f, 2.0f, DARKBLUE);
    Collision c7({76.0f, 6.0f, 0.0f}, {2.0f, 1.0f, 2.0f});
    c7.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c7));

    // Descent platform 2
    Vector3 plat8 = {86.0f, 3.0f, -3.0f};
    DrawCube(plat8, 1.8f, 1.0f, 1.8f, DARKPURPLE);
    Collision c8({86.0f, 3.0f, -3.0f}, {1.8f, 1.0f, 1.8f});
    c8.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c8));

    // Final challenge before finish
    Vector3 plat9 = {96.0f, 7.0f, 1.0f};
    DrawCube(plat9, 1.5f, 1.0f, 1.5f, RED);
    Collision c9({96.0f, 7.0f, 1.0f}, {1.5f, 1.0f, 1.5f});
    c9.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c9));

    // Victory platform
    Vector3 endPos = {108.0f, 2.0f, -1.0f};
    DrawCube(endPos, 5.0f, 1.0f, 5.0f, GOLD);
    Collision endPlatform({108.0f, 2.0f, -1.0f}, {5.0f, 1.0f, 5.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateHardParkourMap()
{
    // Advanced Hard difficulty using Raylib functions
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Compact starting area for hard mode
    DrawCube(startPos, 3.0f, 1.0f, 3.0f, DARKGREEN);
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 3.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Extreme precision challenges
    Vector3 plat1 = {10.0f, 0.0f, 6.0f};
    DrawCube(plat1, 1.2f, 1.0f, 1.2f, DARKBLUE);
    Collision c1({10.0f, 0.0f, 6.0f}, {1.2f, 1.0f, 1.2f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Very high precision platform
    Vector3 plat2 = {18.0f, 8.0f, 4.0f};
    DrawCube(plat2, 0.9f, 1.0f, 0.9f, DARKPURPLE);
    Collision c2({18.0f, 8.0f, 4.0f}, {0.9f, 1.0f, 0.9f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Narrow bridge platform
    Vector3 plat3 = {26.0f, 3.0f, 2.0f};
    DrawCube(plat3, 1.0f, 1.0f, 1.0f, DARKBROWN);
    Collision c3({26.0f, 3.0f, 2.0f}, {1.0f, 1.0f, 1.0f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // Extreme height challenge
    Vector3 plat4 = {34.0f, 12.0f, 0.0f};
    DrawCube(plat4, 0.8f, 1.0f, 0.8f, RED);
    Collision c4({34.0f, 12.0f, 0.0f}, {0.8f, 1.0f, 0.8f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Recovery but still challenging
    Vector3 plat5 = {42.0f, 5.0f, -2.0f};
    DrawCube(plat5, 1.5f, 1.0f, 1.5f, DARKGRAY);
    Collision c5({42.0f, 5.0f, -2.0f}, {1.5f, 1.0f, 1.5f});
    c5.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c5));

    // Another extreme height
    Vector3 plat6 = {50.0f, 15.0f, 1.0f};
    DrawCube(plat6, 0.7f, 1.0f, 0.7f, ORANGE);
    Collision c6({50.0f, 15.0f, 1.0f}, {0.7f, 1.0f, 0.7f});
    c6.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c6));

    // Very narrow connecting platform
    Vector3 plat7 = {58.0f, 8.0f, -1.0f};
    DrawCube(plat7, 1.0f, 1.0f, 1.0f, DARKBLUE);
    Collision c7({58.0f, 8.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
    c7.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c7));

    // Final extreme challenge
    Vector3 plat8 = {66.0f, 18.0f, 2.0f};
    DrawCube(plat8, 0.6f, 1.0f, 0.6f, DARKPURPLE);
    Collision c8({66.0f, 18.0f, 2.0f}, {0.6f, 1.0f, 0.6f});
    c8.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c8));

    // Descent precision platforms
    Vector3 plat9 = {74.0f, 12.0f, 0.0f};
    DrawCube(plat9, 1.2f, 1.0f, 1.2f, DARKBROWN);
    Collision c9({74.0f, 12.0f, 0.0f}, {1.2f, 1.0f, 1.2f});
    c9.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c9));

    Vector3 plat10 = {82.0f, 7.0f, -2.0f};
    DrawCube(plat10, 1.0f, 1.0f, 1.0f, RED);
    Collision c10({82.0f, 7.0f, -2.0f}, {1.0f, 1.0f, 1.0f});
    c10.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c10));

    Vector3 plat11 = {90.0f, 4.0f, 1.0f};
    DrawCube(plat11, 0.8f, 1.0f, 0.8f, DARKGRAY);
    Collision c11({90.0f, 4.0f, 1.0f}, {0.8f, 1.0f, 0.8f});
    c11.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c11));

    // Final platform
    Vector3 endPos = {98.0f, 1.0f, -1.0f};
    DrawCube(endPos, 4.0f, 1.0f, 4.0f, GOLD);
    Collision endPlatform({98.0f, 1.0f, -1.0f}, {4.0f, 1.0f, 4.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateSpeedrunParkourMap()
{
    // Advanced Speedrun map using Raylib functions - optimized for fast times
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Speedrun-optimized starting platform
    DrawCube(startPos, 4.0f, 1.0f, 4.0f, DARKGREEN);
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Fast track platforms - optimized for speed
    Vector3 plat1 = {8.0f, 0.0f, 3.0f};
    DrawCube(plat1, 3.2f, 1.0f, 2.2f, DARKBLUE);
    Collision c1({8.0f, 0.0f, 3.0f}, {3.2f, 1.0f, 2.2f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Quick jump platform
    Vector3 plat2 = {16.0f, 3.5f, 5.0f};
    DrawCube(plat2, 2.8f, 1.0f, 2.4f, DARKPURPLE);
    Collision c2({16.0f, 3.5f, 5.0f}, {2.8f, 1.0f, 2.4f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Sprint platform
    Vector3 plat3 = {24.0f, 1.0f, 6.5f};
    DrawCube(plat3, 3.0f, 1.0f, 2.6f, DARKBROWN);
    Collision c3({24.0f, 1.0f, 6.5f}, {3.0f, 1.0f, 2.6f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // Speed jump platform
    Vector3 plat4 = {32.0f, 4.5f, 5.8f};
    DrawCube(plat4, 2.6f, 1.0f, 2.8f, RED);
    Collision c4({32.0f, 4.5f, 5.8f}, {2.6f, 1.0f, 2.8f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Long platform for building speed
    Vector3 plat5 = {40.0f, 0.5f, 4.2f};
    DrawCube(plat5, 3.4f, 1.0f, 2.0f, DARKGRAY);
    Collision c5({40.0f, 0.5f, 4.2f}, {3.4f, 1.0f, 2.0f});
    c5.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c5));

    // High speed challenge
    Vector3 plat6 = {48.0f, 6.0f, 2.5f};
    DrawCube(plat6, 2.4f, 1.0f, 3.0f, ORANGE);
    Collision c6({48.0f, 6.0f, 2.5f}, {2.4f, 1.0f, 3.0f});
    c6.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c6));

    // Technical precision for speedrunners
    Vector3 plat7 = {56.0f, 2.0f, 0.8f};
    DrawCube(plat7, 2.8f, 1.0f, 1.8f, DARKBLUE);
    Collision c7({56.0f, 2.0f, 0.8f}, {2.8f, 1.0f, 1.8f});
    c7.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c7));

    // Risk-reward platform
    Vector3 plat8 = {64.0f, 7.5f, -1.2f};
    DrawCube(plat8, 2.2f, 1.0f, 2.6f, DARKPURPLE);
    Collision c8({64.0f, 7.5f, -1.2f}, {2.2f, 1.0f, 2.6f});
    c8.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c8));

    // Speed tunnel platform
    Vector3 plat9 = {72.0f, 3.0f, -3.5f};
    DrawCube(plat9, 3.2f, 1.0f, 2.0f, DARKBROWN);
    Collision c9({72.0f, 3.0f, -3.5f}, {3.2f, 1.0f, 2.0f});
    c9.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c9));

    // Advanced speed platform
    Vector3 plat10 = {80.0f, 8.0f, -5.8f};
    DrawCube(plat10, 2.0f, 1.0f, 2.8f, RED);
    Collision c10({80.0f, 8.0f, -5.8f}, {2.0f, 1.0f, 2.8f});
    c10.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c10));

    // Final sprint platforms
    Vector3 plat11 = {88.0f, 4.0f, -4.2f};
    DrawCube(plat11, 2.6f, 1.0f, 2.4f, DARKGRAY);
    Collision c11({88.0f, 4.0f, -4.2f}, {2.6f, 1.0f, 2.4f});
    c11.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c11));

    Vector3 plat12 = {96.0f, 1.5f, -2.5f};
    DrawCube(plat12, 2.4f, 1.0f, 2.2f, DARKBLUE);
    Collision c12({96.0f, 1.5f, -2.5f}, {2.4f, 1.0f, 2.2f});
    c12.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c12));

    // Ultimate speed platform
    Vector3 plat13 = {104.0f, 5.0f, -0.8f};
    DrawCube(plat13, 2.0f, 1.0f, 2.0f, DARKPURPLE);
    Collision c13({104.0f, 5.0f, -0.8f}, {2.0f, 1.0f, 2.0f});
    c13.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c13));

    // Final victory platform
    Vector3 plat14 = {112.0f, 2.0f, 0.5f};
    DrawCube(plat14, 2.8f, 1.0f, 2.8f, DARKBROWN);
    Collision c14({112.0f, 2.0f, 0.5f}, {2.8f, 1.0f, 2.8f});
    c14.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c14));

    // Grand finish platform
    Vector3 endPos = {122.0f, 0.0f, -1.0f};
    DrawCube(endPos, 6.0f, 1.0f, 6.0f, GOLD);
    Collision endPlatform({122.0f, 0.0f, -1.0f}, {6.0f, 1.0f, 6.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateIceTempleMap()
{
    // Ice-themed parkour map using Raylib functions
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Icy starting platform
    DrawCube(startPos, 4.0f, 1.0f, 4.0f, SKYBLUE);
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Ice platforms with slippery theme
    Vector3 icePlat1 = {12.0f, 0.0f, 6.0f};
    DrawCube(icePlat1, 3.0f, 1.0f, 3.0f, Color{150, 200, 255, 255});
    Collision c1({12.0f, 0.0f, 6.0f}, {3.0f, 1.0f, 3.0f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Crystal formations (decorative)
    Vector3 crystal1 = {20.0f, 2.0f, 4.0f};
    DrawCube(crystal1, 0.5f, 4.0f, 0.5f, Color{200, 220, 255, 255});
    Vector3 crystal2 = {22.0f, 2.0f, 4.0f};
    DrawCube(crystal2, 0.5f, 4.0f, 0.5f, Color{200, 220, 255, 255});

    // Floating ice platforms
    Vector3 icePlat2 = {28.0f, 5.0f, 2.0f};
    DrawCube(icePlat2, 2.5f, 1.0f, 2.5f, Color{180, 220, 255, 255});
    Collision c2({28.0f, 5.0f, 2.0f}, {2.5f, 1.0f, 2.5f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Ice bridge
    Vector3 bridge1 = {38.0f, 2.0f, 0.0f};
    DrawCube(bridge1, 4.0f, 1.0f, 1.5f, Color{160, 210, 255, 255});
    Collision c3({38.0f, 2.0f, 0.0f}, {4.0f, 1.0f, 1.5f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // High ice spire platform
    Vector3 icePlat3 = {50.0f, 8.0f, -2.0f};
    DrawCube(icePlat3, 2.0f, 1.0f, 2.0f, Color{190, 230, 255, 255});
    Collision c4({50.0f, 8.0f, -2.0f}, {2.0f, 1.0f, 2.0f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Spiral ice staircase
    for (int i = 0; i < 8; i++)
    {
        float angle = i * PI / 4;
        float radius = 60.0f + i * 2.0f;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        float y = 2.0f + i * 1.5f;

        Vector3 spiralPos = {x, y, z};
        DrawCube(spiralPos, 2.5f, 1.0f, 2.5f, Color{170, 200, 250, 255});
        Collision spiralColl(spiralPos, {2.5f, 1.0f, 2.5f});
        spiralColl.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(std::move(spiralColl));
    }

    // Ice cavern platforms
    Vector3 cavern1 = {75.0f, 3.0f, 5.0f};
    DrawCube(cavern1, 3.0f, 1.0f, 3.0f, Color{140, 190, 240, 255});
    Collision c5({75.0f, 3.0f, 5.0f}, {3.0f, 1.0f, 3.0f});
    c5.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c5));

    // Final frozen platform
    Vector3 endPos = {90.0f, 1.0f, 0.0f};
    DrawCube(endPos, 5.0f, 1.0f, 5.0f, Color{220, 240, 255, 255});
    Collision endPlatform({90.0f, 1.0f, 0.0f}, {5.0f, 1.0f, 5.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateFireTempleMap()
{
    // Fire-themed parkour map using Raylib functions
    Vector3 startPos = {0.0f, 0.0f, 0.0f};

    // Volcanic starting platform
    DrawCube(startPos, 4.0f, 1.0f, 4.0f, Color{50, 25, 25, 255});
    Collision startPlatform({0.0f, 0.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Lava flow platforms
    Vector3 lavaPlat1 = {15.0f, 0.0f, 8.0f};
    DrawCube(lavaPlat1, 3.5f, 1.0f, 3.5f, Color{100, 30, 20, 255});
    Collision c1({15.0f, 0.0f, 8.0f}, {3.5f, 1.0f, 3.5f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // Rising platforms (simulated with stepped design)
    for (int i = 0; i < 6; i++)
    {
        Vector3 risePos = {30.0f + i * 3.0f, 1.0f + i * 2.0f, 5.0f};
        Color fireColor = Color{static_cast<unsigned char>(80 + i * 20), 20, 10, 255};
        DrawCube(risePos, 2.8f, 1.0f, 2.8f, fireColor);
        Collision riseColl(risePos, {2.8f, 1.0f, 2.8f});
        riseColl.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(std::move(riseColl));
    }

    // Fire pit crossing (narrow bridges)
    Vector3 bridge1 = {50.0f, 4.0f, 2.0f};
    DrawCube(bridge1, 6.0f, 1.0f, 1.2f, Color{60, 20, 15, 255});
    Collision c2({50.0f, 4.0f, 2.0f}, {6.0f, 1.0f, 1.2f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Ascending fire platforms
    Vector3 firePlat2 = {65.0f, 8.0f, -1.0f};
    DrawCube(firePlat2, 2.2f, 1.0f, 2.2f, Color{120, 40, 25, 255});
    Collision c3({65.0f, 8.0f, -1.0f}, {2.2f, 1.0f, 2.2f});
    c3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c3));

    // Magma chamber platforms
    Vector3 magma1 = {80.0f, 5.0f, 3.0f};
    DrawCube(magma1, 3.0f, 1.0f, 3.0f, Color{90, 25, 15, 255});
    Collision c4({80.0f, 5.0f, 3.0f}, {3.0f, 1.0f, 3.0f});
    c4.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c4));

    // Final volcanic platform
    Vector3 endPos = {100.0f, 2.0f, 0.0f};
    DrawCube(endPos, 5.0f, 1.0f, 5.0f, Color{70, 20, 10, 255});
    Collision endPlatform({100.0f, 2.0f, 0.0f}, {5.0f, 1.0f, 5.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

void Game::CreateSkyIslandsMap()
{
    // Sky islands floating parkour map using Raylib functions
    Vector3 startPos = {0.0f, 10.0f, 0.0f};

    // Cloud starting platform
    DrawCube(startPos, 4.0f, 1.0f, 4.0f, WHITE);
    Collision startPlatform({0.0f, 10.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    // Floating island chain
    for (int i = 0; i < 10; i++)
    {
        float x = 15.0f + i * 8.0f;
        float y = 10.0f + sin(i * 0.5f) * 3.0f;
        float z = sin(i * 0.3f) * 5.0f;

        Vector3 islandPos = {x, y, z};
        Color islandColor = Color{static_cast<unsigned char>(200 + i * 5), 220, 240, 255};
        DrawCube(islandPos, 3.0f, 1.0f, 3.0f, islandColor);
        Collision islandColl(islandPos, {3.0f, 1.0f, 3.0f});
        islandColl.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(std::move(islandColl));
    }

    // Cloud bridges
    Vector3 cloudBridge1 = {45.0f, 12.0f, 2.0f};
    DrawCube(cloudBridge1, 8.0f, 1.0f, 2.0f, Color{240, 240, 255, 255});
    Collision c1({45.0f, 12.0f, 2.0f}, {8.0f, 1.0f, 2.0f});
    c1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c1));

    // High altitude challenge
    Vector3 highPlat = {70.0f, 18.0f, -3.0f};
    DrawCube(highPlat, 2.5f, 1.0f, 2.5f, Color{220, 230, 250, 255});
    Collision c2({70.0f, 18.0f, -3.0f}, {2.5f, 1.0f, 2.5f});
    c2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(c2));

    // Descending cloud platforms
    for (int i = 0; i < 6; i++)
    {
        Vector3 descendPos = {85.0f + i * 4.0f, 15.0f - i * 1.5f, 1.0f};
        DrawCube(descendPos, 2.8f, 1.0f, 2.8f, Color{210, 220, 245, 255});
        Collision descendColl(descendPos, {2.8f, 1.0f, 2.8f});
        descendColl.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(std::move(descendColl));
    }

    // Final landing platform
    Vector3 endPos = {110.0f, 8.0f, -1.0f};
    DrawCube(endPos, 6.0f, 1.0f, 6.0f, Color{255, 255, 255, 255});
    Collision endPlatform({110.0f, 8.0f, -1.0f}, {6.0f, 1.0f, 6.0f});
    endPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(endPlatform));
}

// ============================================================================
// Editor Map Loading System
// ============================================================================

void Game::LoadEditorMap(const std::string& mapPath)
{
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Loading map from: %s", mapPath.c_str());

    // Clear previous map data
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Clearing previous map data...");
    m_gameMap.Cleanup();
    m_gameMap = GameMap{};

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - File extension: %s", extension.c_str());

    if (extension == "json")
    {
        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Detected JSON format, using MapLoader");

        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Using MapLoader for robust JSON parsing...");
        MapLoader mapLoader;
        m_gameMap = mapLoader.LoadMap(mapPath);

        if (!m_gameMap.objects.empty())
        {
            TraceLog(LOG_INFO, "Game::LoadEditorMap() - MapLoader import successful, processing %d objects", m_gameMap.objects.size());

            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Successfully loaded JSON map with %d objects", m_gameMap.objects.size());
        }
        else
        {
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Failed to load JSON map");
            return;
        }
    }

    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Map loaded, checking object count: %d", m_gameMap.objects.size());
    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Validate map object count to prevent memory issues
    if (m_gameMap.objects.size() > 10000)
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Map has too many objects (%d), limiting to 10000", m_gameMap.objects.size());
        return;
    }

    // Create collision boxes for all objects in the map
    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Creating collision boxes for %d objects", m_gameMap.objects.size());
    for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
    {
        const auto& object = m_gameMap.objects[i];

        // Validate object data before creating collision
        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) || !std::isfinite(object.position.z))
        {
            TraceLog(LOG_WARNING, "Game::LoadEditorMap() - Object %d has invalid position, skipping collision", i);
            continue;
        }

        if (!std::isfinite(object.scale.x) || !std::isfinite(object.scale.y) || !std::isfinite(object.scale.z))
        {
            TraceLog(LOG_WARNING, "Game::LoadEditorMap() - Object %d has invalid scale, skipping collision", i);
            continue;
        }

        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Creating collision for object %d: %s", i, object.name.c_str());
        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Object %d position: (%.2f, %.2f, %.2f)", i, object.position.x, object.position.y, object.position.z);

        Vector3 colliderSize = object.scale;

        // Adjust collider size based on object type
        switch (object.type)
        {
            case MapObjectType::SPHERE:
                // For spheres, use radius for all dimensions
                colliderSize = Vector3{object.radius, object.radius, object.radius};
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Sphere collision: size=(%.2f, %.2f, %.2f)", colliderSize.x, colliderSize.y, colliderSize.z);
                break;
            case MapObjectType::CYLINDER:
                // For cylinders, use radius for x/z and height for y
                colliderSize = Vector3{object.radius, object.height, object.radius};
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Cylinder collision: size=(%.2f, %.2f, %.2f)", colliderSize.x, colliderSize.y, colliderSize.z);
                break;
            case MapObjectType::PLANE:
                // For planes, use size for x/z and small height for y
                colliderSize = Vector3{object.size.x, 0.1f, object.size.y};
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Plane collision: size=(%.2f, %.2f, %.2f)", colliderSize.x, colliderSize.y, colliderSize.z);
                break;
            case MapObjectType::MODEL:
            case MapObjectType::LIGHT: // Handle incorrectly exported MODEL objects as LIGHT type
                // For models, use scale as bounding box
                // Scale represents the size of the model instance
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Model collision: size=(%.2f, %.2f, %.2f)", colliderSize.x, colliderSize.y, colliderSize.z);
                break;
            default:
                // For cubes and other types, use scale as-is
                TraceLog(LOG_INFO, "Game::LoadEditorMap() - Cube/Model collision: size=(%.2f, %.2f, %.2f)", colliderSize.x, colliderSize.y, colliderSize.z);
                break;
        }

        try
        {
            Collision collision(object.position, colliderSize);
            collision.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager.AddCollider(std::move(collision));

            TraceLog(LOG_INFO, "Game::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f)",
                     object.name.c_str(), object.position.x, object.position.y, object.position.z);
        }
        catch (const std::exception& e)
        {
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Failed to create collision for object %s: %s", object.name.c_str(), e.what());
        }
        catch (...)
        {
            TraceLog(LOG_ERROR, "Game::LoadEditorMap() - Unknown error creating collision for object %s", object.name.c_str());
        }
    }

    // Set player start position if specified in map metadata
    if (m_gameMap.metadata.startPosition.x != 0.0f ||
        m_gameMap.metadata.startPosition.y != 0.0f ||
        m_gameMap.metadata.startPosition.z != 0.0f)
    {
        m_player.SetPlayerPosition(m_gameMap.metadata.startPosition);
        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Set player start position to (%.2f, %.2f, %.2f)",
                 m_gameMap.metadata.startPosition.x, m_gameMap.metadata.startPosition.y, m_gameMap.metadata.startPosition.z);
    }

    TraceLog(LOG_INFO, "Game::LoadEditorMap() - Successfully loaded map with %d objects", m_gameMap.objects.size());
}

void Game::RenderEditorMap()
{
    // Render the loaded map objects
    for (const auto& object : m_gameMap.objects)
    {
        // Render based on object type
        switch (object.type)
        {
            case MapObjectType::CUBE:
                DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
                break;

            case MapObjectType::SPHERE:
                DrawSphere(object.position, object.radius, object.color);
                break;

            case MapObjectType::CYLINDER:
                // Draw cylinder using multiple spheres for approximation
                // For better cylinder rendering, you might want to use a 3D model
                DrawSphere(object.position, object.radius, object.color);
                DrawSphere(Vector3{object.position.x, object.position.y + object.height, object.position.z}, object.radius, object.color);
                break;

            case MapObjectType::PLANE:
                // Draw plane as a thin cube
                DrawCube(object.position, object.size.x, 0.1f, object.size.y, object.color);
                break;

            case MapObjectType::MODEL:
            case MapObjectType::LIGHT: // Handle both MODEL and incorrectly exported MODEL objects as LIGHT type
                // For model objects, try to load and render the actual model
                if (!object.modelName.empty())
                {
                    try
                    {
                        Model* model = &m_models.GetModelByName(object.modelName.c_str());
                        if (model && model->meshCount > 0)
                        {
                            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Rendering model: %s at position (%.2f, %.2f, %.2f)",
                                     object.modelName.c_str(), object.position.x, object.position.y, object.position.z);

                            // Apply transformations: scale, rotation, translation
                            Matrix scale = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
                            Matrix rotationX = MatrixRotateX(object.rotation.x * DEG2RAD);
                            Matrix rotationY = MatrixRotateY(object.rotation.y * DEG2RAD);
                            Matrix rotationZ = MatrixRotateZ(object.rotation.z * DEG2RAD);
                            Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);

                            // Combine transformations: scale -> rotate -> translate
                            Matrix transform = MatrixMultiply(scale, rotationX);
                            transform = MatrixMultiply(transform, rotationY);
                            transform = MatrixMultiply(transform, rotationZ);
                            transform = MatrixMultiply(transform, translation);

                            // Apply transformation and draw model
                            model->transform = transform;
                            DrawModel(*model, Vector3{0, 0, 0}, 1.0f, object.color);

                            TraceLog(LOG_INFO, "Game::RenderEditorMap() - Model rendered successfully: %s", object.modelName.c_str());
                        }
                        else
                        {
                            TraceLog(LOG_ERROR, "Game::RenderEditorMap() - Model %s not found or has no meshes!", object.modelName.c_str());
                            // Fallback to cube if model not found
                            DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        TraceLog(LOG_ERROR, "Game::RenderEditorMap() - Exception while rendering model %s: %s", object.modelName.c_str(), e.what());
                        // Fallback to cube if model loading fails
                        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
                    }
                }
                else
                {
                    // No model name specified, draw as cube
                    DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
                }
                break;

            default:
                // Unknown type, draw as cube
                DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
                break;
        }
    }

}
