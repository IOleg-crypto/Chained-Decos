#include "Game.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/GroundColliderFactory.h"
#include "Engine/Engine.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Model/Model.h"
#include "Game/Menu/Menu.h"
#include "Engine/Render/RenderManager.h"
#include "imgui.h"

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
    if (m_collisionManager.GetColliders().size() > 0)
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

    // Initialize menu with engine reference
    m_menu.GetEngine(m_engine);

    // Kernel boot and service registration
    Kernel &kernel = Kernel::GetInstance();
    kernel.Initialize();
    kernel.RegisterService<InputService>(Kernel::ServiceType::Input, std::make_shared<InputService>(&m_engine->GetInputManager()));
    kernel.RegisterService<ModelsService>(Kernel::ServiceType::Models, std::make_shared<ModelsService>(&m_models));
    kernel.RegisterService<WorldService>(Kernel::ServiceType::World, std::make_shared<WorldService>(&m_world));
    // CollisionManager and WorldManager will be registered after creation/initialization

    LoadGameModels();
    InitPlayer();
    InitInput();

    m_isGameInitialized = true;
    TraceLog(LOG_INFO, "Game::Init() - Game components initialized.");
}

void Game::Run()
{
    TraceLog(LOG_INFO, "Game::Run() - Starting game loop...");

    while (!m_engine->ShouldClose())
    {
        Update();
        Render();
    }

    TraceLog(LOG_INFO, "Game::Run() - Game loop ended.");
}

void Game::Update()
{
    // Update engine (handles window and timing)
    m_engine->Update();

    // Update kernel services each frame
    Kernel::GetInstance().Update(GetFrameTime());

    // Handle console input (works in both menu and gameplay)
    if (IsKeyPressed(KEY_GRAVE)) // ~ key
    {
        m_menu.ToggleConsole();
    }
    m_menu.HandleConsoleInput();

    // Only process other input if console is not open
    if (!m_menu.IsConsoleOpen())
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
            UpdatePlayerLogic();
            UpdatePhysicsLogic();
        }
    }
}

void Game::Render()
{
    m_engine->GetRenderManager()->BeginFrame();

    if (m_showMenu)
    {
        m_engine->GetRenderManager()->RenderMenu(m_menu);
    }
    else
    {
        RenderGameWorld();
        RenderGameUI();
    }

    if (m_engine->IsDebugInfoVisible() && !m_showMenu)
    {
        m_engine->GetRenderManager()->RenderDebugInfo(m_player, m_models, m_collisionManager);
    }

    // Render console on top of everything
    m_menu.RenderConsole();

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

void Game::RequestExit() const
{
    m_engine->RequestExit();
    TraceLog(LOG_INFO, "Game exit requested.");
}

bool Game::IsRunning() const
{
    return !m_engine->ShouldClose();
}

void Game::InitInput()
{
    TraceLog(LOG_INFO, "Game::InitInput() - Setting up game-specific input bindings...");

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

    // Create ground using factory
    Collision groundPlane = GroundColliderFactory::CreateDefaultGameGround();
    m_collisionManager.AddCollider(std::move(groundPlane));

    // Create parkour test map based on menu selection
    MenuAction action = m_menu.GetAction();
    if(action == MenuAction::SelectMap1)
    {
        CreateEasyParkourMap();
        m_isGameInitialized = true;
    }
    else if(action == MenuAction::SelectMap2)
    {
        CreateMediumParkourMap();
        m_isGameInitialized = true;
    }
    else if(action == MenuAction::SelectMap3)
    {
        CreateHardParkourMap();
        m_isGameInitialized = true;

    }
    else if(action == MenuAction::StartGameWithMap)
    {
        m_isGameInitialized = true;
        CreateSpeedrunParkourMap();
    }
    else
    {
        m_isGameInitialized = true;

        // Default fallback to original test map
        CreateParkourTestMap();
    }

    // Initialize ground collider first
    m_collisionManager.Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(Kernel::ServiceType::Collision, std::make_shared<CollisionService>(&m_collisionManager));

    // Load model collisions
    m_collisionManager.CreateAutoCollisionsFromModels(m_models);

    // Reinitialize after adding all model colliders
    m_collisionManager.Initialize();

    // Initialize player collision
    auto& playerCollision = m_player.GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(LOG_INFO, "Game::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager.GetColliders().size());
}

void Game::InitPlayer()
{
    TraceLog(LOG_INFO, "Game::InitPlayer() - Initializing player...");

    // Set initial position on the first platform (mix of ground and floating platforms)
    Vector3 safePosition = {0.0f, GameConstants::PLAYER_SAFE_SPAWN_HEIGHT, 0.0f};
    m_player.SetPlayerPosition(safePosition);

    // Setup collision and physics
    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    // Allow physics to determine grounded state; start ungrounded so gravity applies
    m_player.GetPhysics().SetGroundLevel(false);
    m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Load player model with improved error handling
    try
    {
        Model* playerModel = &m_models.GetModelByName("player");
        if (playerModel && playerModel->meshCount > 0)
        {
            m_player.SetPlayerModel(playerModel);
            TraceLog(LOG_INFO, "Game::InitPlayer() - Player model loaded successfully.");
        }
        else
        {
            TraceLog(LOG_ERROR, "Game::InitPlayer() - Player model is invalid or has no meshes");
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
}

void Game::LoadGameModels()
{
    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading game models...");
    const std::string modelsJsonPath = PROJECT_ROOT_DIR "/src/Game/Resource/models.json";
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(50);
    m_models.EnableLOD(true);

    try
    {
        m_models.LoadModelsFromJson(modelsJsonPath);
        m_models.PrintStatistics();
        TraceLog(LOG_INFO, "Game::LoadGameModels() - Models loaded successfully.");

        // Validate that we have essential models
        auto availableModels = m_models.GetAvailableModels();
        bool hasPlayerModel = std::find(availableModels.begin(), availableModels.end(), "player") != availableModels.end();

        if (!hasPlayerModel)
        {
            TraceLog(LOG_WARNING, "Game::LoadGameModels() - Player model not found, player may not render correctly");
        }

        
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Game::LoadGameModels() - Failed to load models: %s", e.what());
        TraceLog(LOG_ERROR, "Game::LoadGameModels() - Game may not function correctly without models");
    }
}

void Game::UpdatePlayerLogic()
{
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

        // Create emergency ground plane if no colliders exist
        try
        {
            Collision plane = GroundColliderFactory::CreateDefaultGameGround();
            m_collisionManager.AddCollider(std::move(plane));
            TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - Created emergency ground plane.");
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
    switch (m_menu.GetAction())
    {
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting singleplayer...");
        m_menu.SetGameInProgress(true);
        ToggleMenu();
        InitCollisions();
        InitPlayer();
        m_isGameInitialized = true; // Mark game as initialized
        m_menu.ResetAction();
        break;

    case MenuAction::ResumeGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Resuming game...");
        m_menu.SetAction(MenuAction::SinglePlayer);
        // Ensure game is properly initialized for resume
        if (!m_isGameInitialized)
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Initializing game for resume...");
            InitCollisions();
            InitPlayer();

        }
        
        else
        {
            // Game is already initialized, just ensure collision system is ready
            if (m_collisionManager.GetColliders().empty())
            {
                TraceLog(LOG_WARNING, "Game::HandleMenuActions() - No colliders found, reinitializing...");
                InitCollisions();
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
    case MenuAction::SelectMap1:
    case MenuAction::SelectMap2:
    case MenuAction::SelectMap3:
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting game with selected map...");
            m_menu.SetGameInProgress(true);
            std::string selectedMap = m_menu.GetSelectedMapName();
            TraceLog(LOG_INFO, "Selected map: %s", selectedMap.c_str());

            // Initialize game components first
            InitCollisions();
            InitPlayer();
            m_isGameInitialized = true; // Mark game as initialized

            // Hide menu and start the game
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
        m_engine->RequestExit();
        m_menu.ResetAction();
        break;

    default:
        break;
    }
}

void Game::RenderGameWorld() {
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
    m_gameMap.Cleanup();
    m_gameMap = GameMap{};

    // Load the new comprehensive map format
    m_gameMap = LoadGameMap(mapPath);

    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_ERROR, "Game::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Create collision boxes for all objects in the map
    for (const auto& object : m_gameMap.objects)
    {
        Vector3 colliderSize = object.scale;

        // Adjust collider size based on object type
        switch (object.type)
        {
            case MapObjectType::SPHERE:
                // For spheres, use radius for all dimensions
                colliderSize = Vector3{object.radius, object.radius, object.radius};
                break;
            case MapObjectType::CYLINDER:
                // For cylinders, use radius for x/z and height for y
                colliderSize = Vector3{object.radius, object.height, object.radius};
                break;
            case MapObjectType::PLANE:
                // For planes, use size for x/z and small height for y
                colliderSize = Vector3{object.size.x, 0.1f, object.size.y};
                break;
            default:
                // For cubes and other types, use scale as-is
                break;
        }

        Collision collision(object.position, colliderSize);
        collision.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(std::move(collision));

        TraceLog(LOG_INFO, "Game::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f)",
                 object.name.c_str(), object.position.x, object.position.y, object.position.z);
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
    // Get camera from player for rendering
    Camera3D camera = m_player.GetCameraController()->GetCamera();

    // Render the loaded map
    RenderGameMap(m_gameMap, camera);

    // Also render any legacy map objects for backward compatibility
    for (const auto& mapObj : m_mapObjects)
    {
        DrawModel(mapObj.loadedModel, Vector3{0, 0, 0}, 1.0f, WHITE);
    }
}
