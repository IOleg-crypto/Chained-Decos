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

Game::Game(Engine *engine) : m_showMenu(true), m_isGameInitialized(false), m_isDebugInfo(true)
{
    m_engine = engine;
    TraceLog(LOG_INFO, "Game class initialized.");
}

Game::~Game() { TraceLog(LOG_INFO, "Game class destructor called."); }

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
                                                   m_showMenu = true;
                                                   EnableCursor();
                                               });

    m_engine->GetInputManager().RegisterAction(KEY_ESCAPE,
                                               [this]
                                               {
                                                   if (!m_showMenu)
                                                   {
                                                       m_menu.ResetAction();
                                                       ToggleMenu();
                                                       EnableCursor();
                                                   }
                                               });
    TraceLog(LOG_INFO, "Game::InitInput() - Game input bindings configured.");
}

void Game::InitCollisions()
{
    TraceLog(LOG_INFO, "Game::InitCollisions() - Initializing collision system...");
    m_collisionManager.ClearColliders();

    // Create ground using factory
    Collision groundPlane = GroundColliderFactory::CreateDefaultGameGround();
    m_collisionManager.AddCollider(std::move(groundPlane));

    // Create parkour test map
    if(m_menu.GetAction() == MenuAction::SelectMap1)
    {
        CreateParkourTestMap();
    }

    // Initialize ground collider first
    m_collisionManager.Initialize();
    // Register collision service once initialized
    Kernel::GetInstance().RegisterService<CollisionService>(Kernel::ServiceType::Collision, std::make_shared<CollisionService>(&m_collisionManager));

    // Load model collisions
    m_collisionManager.CreateAutoCollisionsFromModels(m_models);

    // Load arena model if available
    try
    {
        Model& arenaModel = m_models.GetModelByName("arena_test");
    
        TraceLog(LOG_INFO, "Game::InitCollisions() - Arena model loaded successfully."); 
        // else
        // {
        //     TraceLog(LOG_WARNING, "Game::InitCollisions() - Arena model not found!");
        // }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Game::InitCollisions() - Failed to load arena model: %s", e.what());
    }

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

    // Set initial position above ground (ground is at Y = -10.0f, so 5.0f above ground)
    Vector3 safePosition = {0.0f, 5.0f, 0.0f};
    m_player.SetPlayerPosition(safePosition);

    // Setup collision and physics
    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    // Allow physics to determine grounded state; start ungrounded so gravity applies
    m_player.GetPhysics().SetGroundLevel(false);
    m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Load player model
    try
    {
        Model* playerModel = &m_models.GetModelByName("player");
        m_player.SetPlayerModel(playerModel);
        TraceLog(LOG_INFO, "Game::InitPlayer() - Player model loaded successfully.");

    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Game::InitPlayer() - Failed to load player model: %s", e.what());
    }

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
               safePosition.x, safePosition.y, safePosition.z);

    // Additional safety check - ensure player is properly positioned
    Vector3 currentPos = m_player.GetPlayerPosition();
    TraceLog(LOG_INFO, "Game::InitPlayer() - Player current position: (%.2f, %.2f, %.2f)",
               currentPos.x, currentPos.y, currentPos.z);
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
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Game::LoadGameModels() - Failed to load models: %s", e.what());
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

void Game::UpdatePhysicsLogic()
{
    if (m_collisionManager.GetColliders().empty())
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders available for physics in "
                                "Game::UpdatePhysicsLogic()!");
            warningShown = true;
        }

        // Create emergency ground plane if no colliders exist
        Collision plane = GroundColliderFactory::CreateDefaultGameGround();
        m_collisionManager.AddCollider(std::move(plane));

        TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - Created emergency ground plane.");
    }
}


void Game::HandleMenuActions()
{
    switch (m_menu.GetAction())
    {
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting singleplayer...");
        ToggleMenu();
        InitCollisions();
        InitPlayer();
        m_menu.ResetAction();
        break;

    case MenuAction::StartGameWithMap:
    case MenuAction::SelectMap1:
    case MenuAction::SelectMap2:
    case MenuAction::SelectMap3:
        {
            TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting game with selected map...");
            InitCollisions();
            std::string selectedMap = m_menu.GetSelectedMapName();
            TraceLog(LOG_INFO, "Selected map: %s", selectedMap.c_str());

            // Initialize game components first
            InitCollisions();
            InitPlayer();

            // Hide menu and start the game
            m_showMenu = false;
            HideCursor();

            m_menu.ResetAction();
        }
        break;

    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Exit game requested from menu.");
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

void Game::RenderGameUI() const {
    m_engine->GetRenderManager()->ShowMetersPlayer(m_player);

    static float gameTime = 0.0f;
    gameTime += GetFrameTime();

    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;
    int milliseconds = static_cast<int>((gameTime - static_cast<int>(gameTime)) * 1000);

    std::string timerText = TextFormat("%02d:%02d:%03d", minutes, seconds, milliseconds);

    int timerX = 300;
    int timerY = 20;

    Font fontToUse = (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
                         ? m_engine->GetRenderManager()->GetFont()
                         : GetFontDefault();
    DrawTextEx(fontToUse, timerText.c_str(), {static_cast<float>(timerX), static_cast<float>(timerY)}, 20, 2.0f, WHITE);
}

void Game::CreateParkourTestMap()
{
    Collision startPlatform({0.0f, 2.0f, 0.0f}, {3.0f, 0.5f, 3.0f});
    DrawPlane({0.0f, 2.0f, 0.0f}, {5, 8} , RED);
    startPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(startPlatform));

    Collision platform1({8.0f, 2.0f, 0.0f}, {1.5f, 0.5f, 1.5f});
    platform1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(platform1));
    DrawCube({8.0f, 2.0f, 0.0f}, 1.5f, 0.5f, 1.5f, BLUE);

    Collision platform2({14.0f, 3.0f, 0.0f}, {1.0f, 0.5f, 1.0f});
    platform2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(platform2));
    DrawCube({14.0f, 3.0f, 0.0f}, 1.0f, 0.5f, 1.0f, YELLOW);
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
