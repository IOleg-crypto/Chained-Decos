#include "Game.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/GroundColliderFactory.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Game/Menu/Menu.h"
#include "Engine/Render/RenderManager.h"
#include "imgui.h"

Game::Game(Engine &engine) : m_engine(engine), m_showMenu(true), m_isGameInitialized(false) , m_isDebugInfo(true)
{
    TraceLog(LOG_INFO, "Game class initialized.");
}

Game::~Game() { TraceLog(LOG_INFO, "Game class destructor called."); }
void Game::Init()
{
    TraceLog(LOG_INFO, "Game::Init() - Initializing game components...");

    m_engine.Init();
    m_menu.GetEngine(&m_engine); // To get menu

    LoadGameModels();
    InitCollisions();
    InitPlayer();
    InitInput();

    m_isGameInitialized = true;
    TraceLog(LOG_INFO, "Game::Init() - Game components initialized.");
}

void Game::Run()
{
    TraceLog(LOG_INFO, "Game::Run() - Starting game loop...");
    while (!m_engine.ShouldClose())
    {
        Update();
        Render();
    }
    TraceLog(LOG_INFO, "Game::Run() - Game loop ended.");
}

void Game::Update()
{
    m_engine.Update();

    // Handle console input (works in both menu and gameplay)
    if (IsKeyPressed(KEY_GRAVE)) // ~ key
    {
        m_menu.ToggleConsole();
    }
    m_menu.HandleConsoleInput();

    // Only process other input if console is not open
    if (!m_menu.IsConsoleOpen())
    {
        m_engine.GetInputManager().ProcessInput();
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
    m_engine.GetRenderManager()->BeginFrame();

    if (m_showMenu)
    {
        m_engine.GetRenderManager()->RenderMenu(m_menu);
    }
    else
    {
        RenderGameWorld();
        RenderGameUI();
    }

    if (m_engine.IsDebugInfoVisible() && !m_showMenu)
    {
        m_engine.GetRenderManager()->RenderDebugInfo(m_player, m_models, m_collisionManager);
    }

    // Render console on top of everything
    m_menu.RenderConsole();

    m_engine.GetRenderManager()->EndFrame();
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
    m_engine.RequestExit();
    TraceLog(LOG_INFO, "Game exit requested.");
}

bool Game::IsRunning() const { return !m_engine.ShouldClose(); }

void Game::InitInput()
{
    TraceLog(LOG_INFO, "Game::InitInput() - Setting up game-specific input bindings...");

    m_engine.GetInputManager().RegisterAction(KEY_F1,
                                              [this]
                                              {
                                                  m_showMenu = true;
                                                  EnableCursor();
                                              });

    m_engine.GetInputManager().RegisterAction(KEY_ESCAPE,
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

    // Ensure physics starts with grounded state
    m_player.GetPhysics().SetGroundLevel(true);
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
        m_engine.GetRenderManager()->ShowMetersPlayer(m_player);
        return;
    }

    m_player.Update(m_collisionManager);
    m_engine.GetRenderManager()->ShowMetersPlayer(m_player);
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
        m_engine.RequestExit();
        m_menu.ResetAction();
        break;

    default:
        break;
    }
}

void Game::RenderGameWorld() const {

    m_engine.GetRenderManager()->RenderGame(m_player, m_models, m_collisionManager,
                                           m_engine.IsCollisionDebugVisible());
}

void Game::RenderGameUI() const {
    if (m_engine.GetRenderManager())
    {
        m_engine.GetRenderManager()->ShowMetersPlayer(m_player);
    }

    static float gameTime = 0.0f;
    gameTime += GetFrameTime();

    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;
    int milliseconds = static_cast<int>((gameTime - static_cast<int>(gameTime)) * 1000);

    std::string timerText = TextFormat("%02d:%02d:%03d", minutes, seconds, milliseconds);

    int timerX = 300;
    int timerY = 20;

    Font fontToUse = (m_engine.GetRenderManager() && m_engine.GetRenderManager()->GetFont().texture.id != 0)
                         ? m_engine.GetRenderManager()->GetFont()
                         : GetFontDefault();
    DrawTextEx(fontToUse, timerText.c_str(), {static_cast<float>(timerX), static_cast<float>(timerY)}, 20, 2.0f, WHITE);
}

void Game::CreateParkourTestMap()
{
    TraceLog(LOG_INFO, "Game::CreateParkourTestMap() - Creating parkour test obstacles...");

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

    Collision platform3({18.0f, 4.0f, 0.0f}, {0.8f, 0.5f, 0.8f});
    platform3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(platform3));
    DrawCube({18.0f, 4.0f, 0.0f}, 0.8f, 0.5f, 0.8f, ORANGE);

    Collision highPlatform1({25.0f, 6.0f, 0.0f}, {1.5f, 0.5f, 1.5f});
    highPlatform1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(highPlatform1));
    DrawCube({25.0f, 6.0f, 0.0f}, 1.5f, 0.5f, 1.5f, PURPLE);

    Collision highPlatform2({32.0f, 8.0f, 0.0f}, {1.0f, 0.5f, 1.0f});
    highPlatform2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(highPlatform2));
    DrawCube({32.0f, 8.0f, 0.0f}, 1.0f, 0.5f, 1.0f, PINK);
    
    Collision smallCube1({6.0f, 1.5f, 3.0f}, {0.5f, 0.5f, 0.5f});
    smallCube1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(smallCube1));
    DrawCube({6.0f, 1.5f, 3.0f}, 0.5f, 0.5f, 0.5f, RED);

    Collision smallCube2({10.0f, 2.5f, -2.0f}, {0.4f, 0.4f, 0.4f});
    smallCube2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(smallCube2));
    DrawCube({10.0f, 2.5f, -2.0f}, 0.4f, 0.4f, 0.4f, RED);

    Collision smallCube3({16.0f, 3.5f, 2.5f}, {0.3f, 0.3f, 0.3f});
    smallCube3.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(smallCube3));
    DrawCube({16.0f, 3.5f, 2.5f}, 0.3f, 0.3f, 0.3f, RED);

    Collision mediumCube1({12.0f, 4.0f, -4.0f}, {0.8f, 0.8f, 0.8f});
    mediumCube1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(mediumCube1));
    DrawCube({12.0f, 4.0f, -4.0f}, 0.8f, 0.8f, 0.8f, MAROON);

    Collision mediumCube2({20.0f, 5.0f, 4.0f}, {0.7f, 0.7f, 0.7f});
    mediumCube2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(mediumCube2));
    DrawCube({20.0f, 5.0f, 4.0f}, 0.7f, 0.7f, 0.7f, MAROON);

    Collision bigCube1({28.0f, 7.0f, -3.0f}, {1.2f, 1.2f, 1.2f});
    bigCube1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(bigCube1));
    DrawCube({28.0f, 7.0f, -3.0f}, 1.2f, 1.2f, 1.2f, DARKBROWN);
    

    Collision smallSphere1({5.0f, 1.0f, -5.0f}, {0.6f, 0.6f, 0.6f});
    DrawSphere(smallCube1.GetCenter(), smallCube1.GetSize().x, RED);
    smallSphere1.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(smallSphere1));
    
    Collision smallSphere2({11.0f, 2.0f, 5.0f}, {0.5f, 0.5f, 0.5f});
    smallSphere2.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(smallSphere2));
    
    Collision smallSphere3({17.0f, 3.0f, -6.0f}, {0.4f, 0.4f, 0.4f});
    smallSphere3.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(smallSphere3));
    
    Collision mediumSphere1({13.0f, 4.5f, 6.0f}, {0.8f, 0.8f, 0.8f});
    mediumSphere1.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(mediumSphere1));
    
    Collision mediumSphere2({21.0f, 5.5f, -7.0f}, {0.7f, 0.7f, 0.7f});
    mediumSphere2.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(mediumSphere2));
    
    Collision bigSphere1({29.0f, 7.5f, 7.0f}, {1.0f, 1.0f, 1.0f});
    bigSphere1.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(bigSphere1));
    
    Collision bigSphere2({35.0f, 9.0f, -8.0f}, {1.2f, 1.2f, 1.2f});
    bigSphere2.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(bigSphere2));
    
    
    Collision wall1({22.0f, 3.0f, -10.0f}, {0.5f, 2.0f, 0.5f});
    wall1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(wall1));
    DrawCube({22.0f, 3.0f, -10.0f}, 0.5f, 2.0f, 0.5f, GRAY);

    Collision wall2({22.0f, 3.0f, 10.0f}, {0.5f, 2.0f, 0.5f});
    wall2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(wall2));
    DrawCube({22.0f, 3.0f, 10.0f}, 0.5f, 2.0f, 0.5f, GRAY);

    Collision hangingPlatform({30.0f, 10.0f, 0.0f}, {1.0f, 0.3f, 1.0f});
    hangingPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(hangingPlatform));
    DrawCube({30.0f, 10.0f, 0.0f}, 1.0f, 0.3f, 1.0f, SKYBLUE);

    Collision finishPlatform({40.0f, 12.0f, 0.0f}, {4.0f, 1.0f, 4.0f});
    finishPlatform.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(finishPlatform));
    DrawCube({40.0f, 12.0f, 0.0f}, 4.0f, 1.0f, 4.0f, GOLD);


    Collision barrier1({-2.0f, 1.0f, -10.0f}, {0.2f, 2.0f, 20.0f});
    barrier1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(barrier1));
    DrawCube({-2.0f, 1.0f, -10.0f}, 0.2f, 2.0f, 20.0f, DARKGRAY);

    Collision barrier2({-2.0f, 1.0f, 10.0f}, {0.2f, 2.0f, 20.0f});
    barrier2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(barrier2));
    DrawCube({-2.0f, 1.0f, 10.0f}, 0.2f, 2.0f, 20.0f, DARKGRAY);

    Collision sidePlatform1({-10.0f, 3.0f, 5.0f}, {2.0f, 0.5f, 2.0f});
    sidePlatform1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(sidePlatform1));
    DrawCube({-10.0f, 3.0f, 5.0f}, 2.0f, 0.5f, 2.0f, LIME);

    Collision sidePlatform2({-15.0f, 5.0f, -3.0f}, {1.5f, 0.5f, 1.5f});
    sidePlatform2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(sidePlatform2));
    DrawCube({-15.0f, 5.0f, -3.0f}, 1.5f, 0.5f, 1.5f, LIME);

    Collision bridge1({20.0f, 5.0f, 0.0f}, {8.0f, 0.3f, 1.0f});
    bridge1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(bridge1));
    DrawCube({20.0f, 5.0f, 0.0f}, 8.0f, 0.3f, 1.0f, BROWN);

    Collision bridge2({35.0f, 10.0f, 0.0f}, {6.0f, 0.3f, 1.0f});
    bridge2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(bridge2));
    DrawCube({35.0f, 10.0f, 0.0f}, 6.0f, 0.3f, 1.0f, BROWN);
    
    
    Collision longPlatform1({-5.0f, 1.0f, 0.0f}, {2.0f, 0.3f, 0.5f});
    longPlatform1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(longPlatform1));
    DrawCube({-5.0f, 1.0f, 0.0f}, 2.0f, 0.3f, 0.5f, VIOLET);

    Collision longPlatform2({-8.0f, 2.0f, 0.0f}, {1.5f, 0.3f, 0.4f});
    longPlatform2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(longPlatform2));
    DrawCube({-8.0f, 2.0f, 0.0f}, 1.5f, 0.3f, 0.4f, VIOLET);

    Collision precisionCube1({15.0f, 2.0f, -8.0f}, {0.2f, 0.2f, 0.2f});
    precisionCube1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(precisionCube1));
    DrawCube({15.0f, 2.0f, -8.0f}, 0.2f, 0.2f, 0.2f, MAGENTA);

    Collision precisionCube2({19.0f, 3.0f, 8.0f}, {0.25f, 0.25f, 0.25f});
    precisionCube2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(precisionCube2));
    DrawCube({19.0f, 3.0f, 8.0f}, 0.25f, 0.25f, 0.25f, MAGENTA);
    
    Collision trickySphere1({23.0f, 4.0f, -12.0f}, {0.3f, 0.3f, 0.3f});
    trickySphere1.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(trickySphere1));
    
    Collision trickySphere2({27.0f, 5.0f, 12.0f}, {0.35f, 0.35f, 0.35f});
    trickySphere2.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(trickySphere2));
    

    Collision visualPlatform1({5.0f, 1.5f, 8.0f}, {2.5f, 0.4f, 0.8f});
    visualPlatform1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(visualPlatform1));
    DrawCube({5.0f, 1.5f, 8.0f}, 2.5f, 0.4f, 0.8f, BEIGE);

    Collision visualPlatform2({25.0f, 4.5f, -5.0f}, {1.8f, 0.4f, 1.8f});
    visualPlatform2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(visualPlatform2));
    DrawCube({25.0f, 4.5f, -5.0f}, 1.8f, 0.4f, 1.8f, BEIGE);

    Collision displaySphere1({8.0f, 3.0f, 6.0f}, {0.8f, 0.8f, 0.8f});
    displaySphere1.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(displaySphere1));
    DrawSphere({8.0f, 3.0f, 6.0f}, 0.8f, WHITE);

    Collision displaySphere2({32.0f, 6.0f, -4.0f}, {0.6f, 0.6f, 0.6f});
    DrawSphere(displaySphere2.GetCenter(), displaySphere2.GetSize().x, GREEN);
    displaySphere2.SetCollisionType(CollisionType::BVH_ONLY);
    m_collisionManager.AddCollider(std::move(displaySphere2));

    Collision demoCube1({12.0f, 2.0f, 10.0f}, {0.5f, 0.5f, 0.5f});
    demoCube1.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(demoCube1));
    DrawCube({12.0f, 2.0f, 10.0f}, 0.5f, 0.5f, 0.5f, LIGHTGRAY);

    Collision demoCube2({28.0f, 5.0f, 8.0f}, {0.7f, 0.7f, 0.7f});
    demoCube2.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(demoCube2));
    DrawCube({28.0f, 5.0f, 8.0f}, 0.7f, 0.7f, 0.7f, LIGHTGRAY);

    TraceLog(LOG_INFO, "Game::CreateParkourTestMap() - Created %zu parkour obstacles",
              m_collisionManager.GetColliders().size() - 1); // -1 for ground plane
}
