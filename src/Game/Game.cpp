#include "Game/Game.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Game/Menu/Menu.h"
#include "imgui.h"
#include "raylib.h"

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

    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
    Collision groundPlane{groundCenter, groundSize};
    groundPlane.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(std::move(groundPlane));

    m_collisionManager.CreateAutoCollisionsFromModels(m_models);
    m_collisionManager.Initialize();
    TraceLog(LOG_INFO, "Game::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager.GetColliders().size());
}

void Game::InitPlayer() const
{
    TraceLog(LOG_INFO, "Game::InitPlayer() - Initializing player...");
    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    Vector3 safePosition = {0.0f, 2.0f, 0.0f};
    m_player.SetPlayerPosition(safePosition);

    TraceLog(LOG_INFO, "Game::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
             safePosition.x, safePosition.y, safePosition.z);
}

void Game::LoadGameModels()
{
    TraceLog(LOG_INFO, "Game::LoadGameModels() - Loading game models...");
    const std::string modelsJsonPath = PROJECT_ROOT_DIR "/src/models.json";
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
        Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
        Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
        Collision plane{groundCenter, groundSize};
        plane.SetCollisionType(CollisionType::AABB_ONLY);
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

void Game::RenderGameUI() {}