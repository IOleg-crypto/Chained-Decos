#include "Game.h"
#include "imgui.h"
#include "raylib.h"

#include "World/World/Physics.h"

Game::Game(Engine &engine)
    : m_engine(engine), m_menu(&m_engine), m_showMenu(true), m_isGameInitialized(false)
{
    TraceLog(LOG_INFO, "Game class initialized.");
}

Game::~Game()
{

    TraceLog(LOG_INFO, "Game class destructor called.");
}

void Game::Init()
{
    TraceLog(LOG_INFO, "Game::Init() - Initializing game components...");


    m_engine.Init();


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
    // Обробка вводу на рівні рушія (F2/F3)
    m_engine.Update();

    // Обробка вводу на рівні гри
    HandleKeyboardShortcuts(); // Якщо є специфічні для гри клавіатурні скорочення
    m_engine.GetInputManager().ProcessInput();

    // Обробка меню або ігрової логіки
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
    // Починаємо кадр через RenderManager рушія
    m_engine.GetRenderManager().BeginFrame();

    // Рендеринг в залежності від стану меню
    if (m_showMenu)
    {
        m_engine.GetRenderManager().RenderMenu(m_menu);
    }
    else
    {
        RenderGameWorld();
        RenderGameUI();
    }

    // Рендеринг налагоджувальної інформації рушія
    if (m_engine.IsDebugInfoVisible())
    {
        // Тут можна додати рендеринг специфічної ігрової налагоджувальної інформації
        m_engine.GetRenderManager().RenderDebugInfo(m_player, m_models, m_collisionManager);
    }

    // Завершуємо кадр через RenderManager рушія
    m_engine.GetRenderManager().EndFrame();
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

void Game::RequestExit()
{

    m_engine.RequestExit();
    TraceLog(LOG_INFO, "Game exit requested.");
}

bool Game::IsRunning() const
{
    return !m_engine.ShouldClose(); // Перевіряємо лише прапорець рушія
}

void Game::InitInput()
{
    TraceLog(LOG_INFO, "Game::InitInput() - Setting up game-specific input bindings...");

    // Приклад реєстрації ігрових дій (не системних)
    m_engine.GetInputManager().RegisterAction(KEY_F1,
                                              [this]()
                                              {
                                                  m_showMenu = true;
                                                  EnableCursor();
                                              });

    m_engine.GetInputManager().RegisterAction(KEY_ESCAPE,
                                              [this]()
                                              {
                                                  if (!m_showMenu)
                                                  {
                                                      m_menu.ResetAction();
                                                      ToggleMenu();
                                                      EnableCursor();
                                                  }
                                              });
    // Додайте інші ігрові дії тут

    TraceLog(LOG_INFO, "Game::InitInput() - Game input bindings configured.");
}

void Game::InitCollisions()
{
    TraceLog(LOG_INFO, "Game::InitCollisions() - Initializing collision system...");
    m_collisionManager.ClearColliders();

    // Створення базової площини землі
    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
    Collision groundPlane{groundCenter, groundSize};
    groundPlane.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(groundPlane);

    // Створення колізій для моделей (моделі повинні бути завантажені)
    m_collisionManager.CreateAutoCollisionsFromModels(m_models);
    m_collisionManager.Initialize();
    TraceLog(LOG_INFO, "Game::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager.GetColliders().size());
}

void Game::InitPlayer()
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
    const std::string modelsJsonPath =
        PROJECT_ROOT_DIR "/src/models.json";
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
        m_engine.GetRenderManager().ShowMetersPlayer(m_player);
        return;
    }

    m_player.Update(m_collisionManager);
    m_engine.GetRenderManager().ShowMetersPlayer(m_player);
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
        m_collisionManager.AddCollider(plane);

        TraceLog(LOG_WARNING, "Game::UpdatePhysicsLogic() - Created emergency ground plane.");
    }
    // Physics logic for other game objects can go here
}

void Game::HandleKeyboardShortcuts()
{
    // Engine-level shortcuts are handled by Engine::HandleEngineInput()
    // Game-specific shortcuts can be handled here or registered in InputManager
}

void Game::HandleMenuActions()
{
    switch (m_menu.GetAction())
    {
    case MenuAction::SinglePlayer:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Starting singleplayer...");
        ToggleMenu(); // Закриваємо меню
        // Тут можна додати логіку для скидання стану гри, якщо це новий запуск
        InitCollisions(); // Переініціалізація колізій для нової гри
        InitPlayer();     // Переініціалізація гравця
        m_menu.ResetAction();
        break;

    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Game::HandleMenuActions() - Exit game requested from menu.");
        RequestExit();
        m_menu.ResetAction();
        break;

    default:
        break;
    }
}

void Game::RenderGameWorld()
{
    // Основний рендеринг ігрового світу
    // Наприклад, рендеринг гравця, моделей, ландшафту тощо
    m_engine.GetRenderManager().RenderGame(m_player, m_models, m_collisionManager,
                                           m_engine.IsCollisionDebugVisible());
}

void Game::RenderGameUI()
{
    // Рендеринг елементів ігрового інтерфейсу (HUD, тощо)
    // m_engine.GetRenderManager().RenderHUD(m_player);
}