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

    m_engine.GetInputManager().ProcessInput();

    if (m_showMenu)
    {
        HandleMenuActions();
    }
    else
    {
        UpdatePlayerLogic();
        UpdatePhysicsLogic();
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

    // Initialize ground collider first
    m_collisionManager.Initialize();

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

void Game::InitPlayer() const
{
    TraceLog(LOG_INFO, "Game::InitPlayer() - Initializing player...");
    
    // Set initial position before anything else
    Vector3 safePosition = {0.0f, 5.0f, 0.0f}; 
    m_player.SetPlayerPosition(safePosition);
    
    // Setup collision and physics
    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
             safePosition.x, safePosition.y, safePosition.z);
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

// Can be used for dynamic binding of menu
// void Game::HandleKeyboardShortcuts()
// {
//     // Engine-level shortcuts are handled by Engine::HandleEngineInput()
//     // Game-specific shortcuts can be handled here or registered in InputManager
// }

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
