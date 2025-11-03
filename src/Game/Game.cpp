#include "Game.h"
#include "Managers/MapManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/StateManager.h"
#include "Managers/GameRenderHelpers.h"
#include "Managers/PlayerManager.h"
#include "Managers/UpdateManager.h"
#include "Managers/GameRenderManager.h"
#include "Managers/MenuActionHandler.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"
#include "Engine/Engine.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Module/ModuleManager.h"
#include "Engine/Map/MapLoader.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Game/Menu/Menu.h"
#include "Game/Modules/PlayerModule.h"
#include "Game/Modules/MapModule.h"
#include "Game/Modules/MenuModule.h"
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

    if (!m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        TraceLog(LOG_INFO, "Game::Cleanup() - Collision system cleared");
    }

    m_player->SetPlayerPosition({0.0f, 0.0f, 0.0f});
    m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    if (!m_mapManager->GetGameMap().objects.empty())
    {
        m_mapManager->GetGameMap().Cleanup();
        TraceLog(LOG_INFO, "Game::Cleanup() - Editor map cleared");
    }

    m_showMenu = true;
    m_isGameInitialized = false;
    m_menu->SetGameInProgress(false);

    TraceLog(LOG_INFO, "Game::Cleanup() - Game resources cleaned up successfully");
}

Game::~Game()
{
    TraceLog(LOG_INFO, "Game class destructor called.");
}

void Game::Init(int argc, char *argv[])
{
    TraceLog(LOG_INFO, "Game::Init() - Initializing game components...");

    GameConfig config = CommandLineHandler::ParseArguments(argc, argv);

    if (config.developer)
    {
        CommandLineHandler::ShowConfig(config);
    }

    InitializeCoreServices(config);
    InitializeGameComponents();
    RegisterCoreKernelServices();
    InitializeManagers();
    RegisterManagerKernelServices();
    RegisterModules();
    
    if (m_engine && m_engine->GetModuleManager()) {
        m_engine->GetModuleManager()->InitializeAllModules();
    }
    
    InitPlayer();
    InitInput();

    m_isGameInitialized = true;
    TraceLog(LOG_INFO, "Game::Init() - Game components initialized.");

    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(m_icon);
    UnloadImage(m_icon);
}

void Game::InitializeCoreServices(const GameConfig& config)
{
    TraceLog(LOG_INFO, "Game::InitializeCoreServices() - Setting up core services...");

    auto renderManager = std::make_shared<RenderManager>();
    auto inputManager = std::make_shared<InputManager>();

    m_kernel = std::make_unique<Kernel>();
    m_kernel->Initialize();

    m_engine = std::make_unique<Engine>(config.width, config.height, renderManager, inputManager,
                                        m_kernel.get());
    m_engine->Init();
    SetTraceLogLevel(LOG_INFO);

    TraceLog(LOG_INFO, "Game::InitializeCoreServices() - Core services initialized.");
}

void Game::InitializeGameComponents()
{
    TraceLog(LOG_INFO, "Game::InitializeGameComponents() - Creating game components...");

    m_player = std::make_unique<Player>();
    m_collisionManager = std::make_unique<CollisionManager>();
    m_models = std::make_unique<ModelLoader>();
    m_world = std::make_unique<WorldManager>();
    m_menu = std::make_unique<Menu>();

    TraceLog(LOG_INFO, "Game::InitializeGameComponents() - About to initialize menu...");
    m_menu->Initialize(m_engine.get());
    m_menu->SetKernel(m_kernel.get());
    m_menu->SetGame(this);
    TraceLog(LOG_INFO, "Game::InitializeGameComponents() - Menu initialized.");
}

void Game::InitializeManagers()
{
    TraceLog(LOG_INFO, "Game::InitializeManagers() - Creating manager components...");

    m_mapManager = std::make_unique<MapManager>(m_player.get(), m_collisionManager.get(), 
                                                  m_models.get(), m_engine->GetRenderManager(), 
                                                  m_kernel.get(), m_menu.get());
    m_modelManager = std::make_unique<ResourceManager>(m_models.get());
    m_stateManager = std::make_unique<StateManager>(m_player.get(), m_menu.get());
    m_renderHelper = std::make_unique<GameRenderHelpers>(m_collisionManager.get());
    m_playerManager = std::make_unique<PlayerManager>(m_player.get(), m_collisionManager.get(),
                                                       m_models.get(), m_engine.get(), m_mapManager.get());
    m_updateManager = std::make_unique<UpdateManager>(m_collisionManager.get(), m_mapManager.get());
    m_gameRenderManager = std::make_unique<GameRenderManager>(m_player.get(), m_engine.get(),
                                                                m_models.get(), m_collisionManager.get(),
                                                                m_mapManager.get());
    m_menuActionHandler = std::make_unique<MenuActionHandler>(m_kernel.get(),
                                                               &m_showMenu, &m_isGameInitialized);
    
    TraceLog(LOG_INFO, "Game::InitializeManagers() - Manager components initialized.");
}

void Game::RegisterCoreKernelServices()
{
    TraceLog(LOG_INFO, "Game::RegisterCoreKernelServices() - Registering core services...");
    
    m_kernel->RegisterService<CollisionService>(Kernel::ServiceType::Collision,
                                                 std::make_shared<CollisionService>(m_collisionManager.get()));
    
    m_kernel->RegisterService<ModelsService>(Kernel::ServiceType::Models,
                                             std::make_shared<ModelsService>(m_models.get()));
    
    m_kernel->RegisterService<WorldService>(Kernel::ServiceType::World,
                                            std::make_shared<WorldService>(m_world.get()));
    
    m_kernel->RegisterService<PlayerService>(Kernel::ServiceType::Player,
                                             std::make_shared<PlayerService>(m_player.get()));
    
    m_kernel->RegisterService<MenuService>(Kernel::ServiceType::Menu,
                                           std::make_shared<MenuService>(m_menu.get()));
    
    m_kernel->RegisterService<GameService>(Kernel::ServiceType::Game,
                                            std::make_shared<GameService>(this));
    
    if (m_engine)
    {
        TraceLog(LOG_INFO, "Game::RegisterCoreKernelServices() - Engine services already registered.");
    }
    else
    {
        TraceLog(LOG_WARNING,
                 "Game::RegisterCoreKernelServices() - No engine provided, skipping engine-dependent services");
    }
    
    TraceLog(LOG_INFO, "Game::RegisterCoreKernelServices() - Core services registered.");
}

void Game::RegisterManagerKernelServices()
{
    TraceLog(LOG_INFO, "Game::RegisterManagerKernelServices() - Registering manager services...");
    
    m_kernel->RegisterService<MapManagerService>(Kernel::ServiceType::MapManager,
                                                  std::make_shared<MapManagerService>(m_mapManager.get()));
    
    m_kernel->RegisterService<ResourceManagerService>(Kernel::ServiceType::ResourceManager,
                                                       std::make_shared<ResourceManagerService>(m_modelManager.get()));
    
    m_kernel->RegisterService<PlayerManagerService>(Kernel::ServiceType::PlayerManager,
                                                     std::make_shared<PlayerManagerService>(m_playerManager.get()));
    
    TraceLog(LOG_INFO, "Game::RegisterManagerKernelServices() - Manager services registered.");
}

void Game::RegisterModules()
{
    TraceLog(LOG_INFO, "Game::RegisterModules() - Registering game modules...");
    
    if (m_engine) {
        m_engine->RegisterModule(std::make_unique<MapModule>());
        m_engine->RegisterModule(std::make_unique<PlayerModule>());
        m_engine->RegisterModule(std::make_unique<MenuModule>());
        
        TraceLog(LOG_INFO, "Game::RegisterModules() - Modules registered. They will be initialized by Engine.");
    } else {
        TraceLog(LOG_WARNING, "Game::RegisterModules() - No engine available, cannot register modules");
    }
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
    if (m_engine)
    {
        m_engine->Update();
        m_engine->GetInputManager().ProcessInput();
    }

    m_kernel->Update(GetFrameTime());

    if (IsKeyPressed(KEY_GRAVE))
    {
        m_menu->ToggleConsole();
    }

    if (m_showMenu)
    {
        HandleMenuActions();
    }
    else
    {
        bool consoleOpen = m_menu->GetConsoleManager() && m_menu->GetConsoleManager()->IsConsoleOpen();
        if (!consoleOpen)
        {
            UpdatePlayerLogic();
            UpdatePhysicsLogic();
        }
        else
        {
            const ImGuiIO &io = ImGui::GetIO();
            if (io.WantCaptureMouse)
            {
                m_player->GetCameraController()->UpdateCameraRotation();
                m_player->GetCameraController()->UpdateMouseRotation(
                    m_player->GetCameraController()->GetCamera(), m_player->GetMovement()->GetPosition());
                m_player->GetCameraController()->Update();
            }
            m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
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
        RenderGameWorld();
        RenderGameUI();
    }

    if (m_engine->IsDebugInfoVisible() && !m_showMenu)
    {
        m_engine->GetRenderManager()->RenderDebugInfo(*m_player->GetRenderable(), *m_models, *m_collisionManager);
    }

    if (!m_showMenu && m_menu->GetConsoleManager() && m_menu->GetConsoleManager()->IsConsoleOpen())
    {
        rlImGuiBegin();
        m_menu->GetConsoleManager()->RenderConsole();
        rlImGuiEnd();
    }

    m_engine->GetRenderManager()->EndFrame();
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

void Game::EnableCursor()
{
    if (m_engine)
    {
        TraceLog(LOG_INFO, "Game::EnableCursor() - Cursor enabled");
    }
}

void Game::HideCursor()
{
    if (m_engine)
    {
        TraceLog(LOG_INFO, "Game::HideCursor() - Cursor hidden");
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
    return false;
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
                                                       SaveGameState();
                                                       m_menu->ResetAction();
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
    m_playerManager->InitPlayer();
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
    m_playerManager->UpdatePlayerLogic();
}

void Game::UpdatePhysicsLogic()
{
    m_updateManager->UpdatePhysicsLogic();
}

void Game::HandleMenuActions()
{
    m_menuActionHandler->HandleMenuActions();
}


void Game::RenderGameWorld()
{
    m_gameRenderManager->RenderGameWorld();
}

void Game::CreatePlatform(const Vector3 &position, const Vector3 &size, Color color,
                          CollisionType collisionType)
{
    m_renderHelper->CreatePlatform(position, size, color, collisionType);
}

float Game::CalculateDynamicFontSize(float baseSize)
{
    return GameRenderHelpers::CalculateDynamicFontSize(baseSize);
}

void Game::RenderGameUI() const
{
    m_gameRenderManager->RenderGameUI();
}

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

Menu &Game::GetMenu() { return *m_menu; }
