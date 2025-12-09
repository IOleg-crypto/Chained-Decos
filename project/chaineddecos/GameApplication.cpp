#include "GameApplication.h"
#include "Systems/MapSystem/LevelManager.h"
// #include "Systems/PlayerSystem/PlayerController.h"
// #include "Systems/RenderingSystem/RenderingSystem.h"
#include "Systems/UIController/UIManager.h"
#include "components/input/Core/InputManager.h"
#include "components/physics/collision/Core/CollisionManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/config/Core/ConfigManager.h"
#include "core/ecs/Examples.h"
#include "core/ecs/Systems.h"
#include "core/engine/EngineApplication.h"
#include "core/object/module/Core/ModuleManager.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/main/Core/World.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"

#include "imgui.h"
#include "rlImGui.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <raylib.h>

GameApplication::GameApplication(int argc, char *argv[])
    : m_showMenu(true), m_isGameInitialized(false), m_cursorDisabled(false),
      m_showDebugCollision(false), m_showDebugStats(false)
{
    // Parse command line arguments
    m_gameConfig = CommandLineHandler::ParseArguments(argc, argv);
}

GameApplication::~GameApplication()
{
    TraceLog(LOG_INFO, "GameApplication destructor called.");
}

void GameApplication::OnConfigure(EngineConfig &config)
{
    TraceLog(LOG_INFO, "[GameApplication] Pre-initialization...");
    SetTraceLogLevel(LOG_INFO);

    // Load config from game.cfg BEFORE setting window size
    ConfigManager configManager;
    bool configLoaded = false;

    // Try loading from game.cfg using PROJECT_ROOT_DIR (where the game will be installed)
    std::string configPath = std::string(PROJECT_ROOT_DIR) + "/game.cfg";
    if (configManager.LoadFromFile(configPath))
    {
        TraceLog(LOG_INFO, "[GameApplication] Loaded config from %s", configPath.c_str());
        configLoaded = true;
    }
    else
    {
        TraceLog(LOG_WARNING, "[GameApplication] Could not load game.cfg, using defaults");
    }

    // Get resolution from config (if not specified in command line)
    int width = m_gameConfig.width;
    int height = m_gameConfig.height;

    // If resolution not specified in command line (using defaults)
    // and config loaded, use values from config
    if ((width == 1280 && height == 720) && configLoaded)
    {
        configManager.GetResolution(width, height);
        TraceLog(LOG_INFO, "[GameApplication] Using resolution from config: %dx%d", width, height);
    }

    // Also check fullscreen from config
    if (configLoaded && !m_gameConfig.fullscreen)
    {
        m_gameConfig.fullscreen = configManager.IsFullscreen();
    }

    if (m_gameConfig.developer)
    {
        CommandLineHandler::ShowConfig(m_gameConfig);
    }

    TraceLog(LOG_INFO, "[GameApplication] Window config: %dx%d (fullscreen: %s)", width, height,
             m_gameConfig.fullscreen ? "yes" : "no");

    // Update EngineConfig
    config.width = width;
    config.height = height;
    config.title = "Chained Decos";
    config.fullscreen = m_gameConfig.fullscreen;
    config.vsync = true;
    config.enableAudio = true;
}

void GameApplication::OnRegister()
{
    TraceLog(LOG_INFO, "[GameApplication] Registering services and modules...");

    auto &engine = Engine::Instance();

    // 1. Initialize Core Services (Legacy - to be removed later)
    m_collisionManager = std::make_shared<CollisionManager>();
    m_models = std::make_shared<ModelLoader>();
    m_world = std::make_shared<WorldManager>();

    // Register core services
    engine.RegisterService<CollisionManager>(m_collisionManager);
    engine.RegisterService<ModelLoader>(m_models);
    engine.RegisterService<WorldManager>(m_world);

    TraceLog(LOG_INFO, "[GameApplication] Core engine services registered.");

    // 2. Register Game Systems (Modules)
    // Legacy modules - keeping LevelManager and UIManager for now
    engine.RegisterModule(std::make_unique<LevelManager>());
    engine.RegisterModule(std::make_unique<UIManager>());

    // PlayerController and RenderingSystem are replaced by ECS Systems
    // engine.RegisterModule(std::make_unique<PlayerController>());
    // engine.RegisterModule(std::make_unique<RenderingSystem>());

    TraceLog(LOG_INFO, "[GameApplication] Game systems registered.");
}

void GameApplication::OnStart()
{
    TraceLog(LOG_INFO, "[GameApplication] Starting game...");

    // Initialize Static Singletons
    if (AudioManager::Get().Initialize())
    {
        TraceLog(LOG_INFO, "[GameApplication] AudioManager initialized");
        AudioManager::Get().LoadSound("player_fall",
                                      PROJECT_ROOT_DIR "\\resources\\audio\\fallingplayer.wav");
    }

    InputManager::Get().Initialize();

    // Initialize RenderManager with config
    // RenderManager::Get().Initialize(m_gameConfig.width, m_gameConfig.height, "Chained Decos"); //
    // Moved to EngineApplication

    // Setup ImGui style for menu (after ImGui is initialized by RenderManager)
    auto *engine = &Engine::Instance();
    if (engine->GetModuleManager())
    {
        auto *uiModule = engine->GetModuleManager()->GetModule("UI");
        if (uiModule)
        {
            UIManager *uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager && uiManager->GetMenu())
            {
                uiManager->GetMenu()->SetupStyle();
                TraceLog(LOG_INFO, "[GameApplication] ImGui style configured");
            }
        }
    }

    // Initialize ECS
    REGISTRY.clear();

    // Explicitly load player model
    if (m_models)
    {
        std::string playerModelPath = std::string(PROJECT_ROOT_DIR) + "/resources/player_low.glb";
        if (m_models->LoadSingleModel("player_low", playerModelPath))
        {
            TraceLog(LOG_INFO, "[GameApplication] Loaded player model: %s",
                     playerModelPath.c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "[GameApplication] Failed to load player model: %s",
                     playerModelPath.c_str());
        }
    }

    // Try to get player model from loader
    Model *playerModelPtr = nullptr;
    if (m_models)
    {
        // "player_low" is loaded by LevelManager/ModelLoader according to logs
        auto modelOpt = m_models->GetModelByName("player_low");
        if (modelOpt.has_value())
        {
            playerModelPtr = &modelOpt.value().get();
        }
    }

    // Load and Apply Shader
    {
        std::string vsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.vs";
        std::string fsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.fs";

        m_playerShader = LoadShader(vsPath.c_str(), fsPath.c_str());
        m_shaderLoaded = (m_playerShader.id != 0);

        if (m_shaderLoaded)
        {
            m_locFallSpeed = GetShaderLocation(m_playerShader, "fallSpeed");
            m_locTime = GetShaderLocation(m_playerShader, "time");
            m_locWindDir = GetShaderLocation(m_playerShader, "windDirection");

            // Set default values
            float defaultFallSpeed = 0.0f;
            SetShaderValue(m_playerShader, m_locFallSpeed, &defaultFallSpeed, SHADER_UNIFORM_FLOAT);

            Vector3 defaultWind = {1.0f, 0.0f, 0.5f};
            SetShaderValue(m_playerShader, m_locWindDir, &defaultWind, SHADER_UNIFORM_VEC3);

            // Assign shader to model
            if (playerModelPtr)
            {
                playerModelPtr->materials[0].shader = m_playerShader;
                TraceLog(LOG_INFO,
                         "[GameApplication] Applied player_effect shader to player_low model");
            }
            else
            {
                m_playerModel.materials[0].shader = m_playerShader;
                TraceLog(LOG_INFO,
                         "[GameApplication] Applied player_effect shader to fallback model");
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "[GameApplication] Failed to load player_effect shader");
        }
    }

    // Check for save file
    Vector3 spawnPos = {0, 2, 0};
    float savedTimer = 0.0f;
    float savedMaxHeight = 0.0f;
    bool hasSave = false;

    std::string savePath = std::string(PROJECT_ROOT_DIR) + "/savegame.dat";
    std::ifstream saveFile(savePath);
    if (saveFile.is_open())
    {
        if (saveFile >> spawnPos.x >> spawnPos.y >> spawnPos.z >> savedTimer >> savedMaxHeight)
        {
            hasSave = true;
            TraceLog(LOG_INFO, "[GameApplication] Found save file. Loading at: %.2f, %.2f, %.2f",
                     spawnPos.x, spawnPos.y, spawnPos.z);

            // Restore game state flags so "Resume" works
            m_isGameInitialized = true;

            auto *uiModule = Engine::Instance().GetModuleManager()->GetModule("UI");
            if (uiModule)
            {
                auto *uiManager = dynamic_cast<UIManager *>(uiModule);
                if (uiManager && uiManager->GetMenu())
                {
                    uiManager->GetMenu()->SetGameInProgress(true);
                }
            }
        }
        saveFile.close();
    }

    if (playerModelPtr)
    {
        TraceLog(LOG_INFO, "[GameApplication] Using existing model 'player_low'");
        m_playerEntity = ECSExamples::CreatePlayer(spawnPos, playerModelPtr);
    }
    else
    {
        TraceLog(LOG_WARNING, "[GameApplication] 'player_low' not found, using default cube.");
        m_playerModel = LoadModelFromMesh(GenMeshCube(0.8f, 1.8f, 0.8f));
        m_playerEntity = ECSExamples::CreatePlayer(spawnPos, &m_playerModel);
    }

    // Apply saved stats
    if (hasSave && REGISTRY.valid(m_playerEntity))
    {
        auto &player = REGISTRY.get<PlayerComponent>(m_playerEntity);
        player.runTimer = savedTimer;
        player.maxHeight = savedMaxHeight;
    }

    TraceLog(LOG_INFO, "[GameApplication] ECS Player entity created");

    // Apply visual offset to player render component
    if (REGISTRY.valid(m_playerEntity) && REGISTRY.all_of<RenderComponent>(m_playerEntity))
    {
        auto &renderComp = REGISTRY.get<RenderComponent>(m_playerEntity);
        // Player::MODEL_Y_OFFSET is -1.0f, which corrects the visual position relative to physics
        renderComp.offset = {0.0f, Player::MODEL_Y_OFFSET, 0.0f};
        TraceLog(LOG_INFO, "[GameApplication] Set player visual offset to (0, %.2f, 0)",
                 Player::MODEL_Y_OFFSET);
    }

    // Initial state - show menu
    m_showMenu = true;

    // Initialize cursor state
    m_cursorDisabled = false;
    EnableCursor();

    // Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

    // Initialize input
    InitInput();

    // Game is not initialized until a map is selected
    m_isGameInitialized = false;

    // Set window icon
    Image m_icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecos.jpg");
    ImageFormat(&m_icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(m_icon);
    UnloadImage(m_icon);

    // Apply fullscreen
    /*
    if (m_gameConfig.fullscreen && !IsWindowFullscreen())
    {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }
    */

    TraceLog(LOG_INFO, "[GameApplication] Game application initialized with ECS.");
}

void GameApplication::OnUpdate(float deltaTime)
{
    // Update Input
    InputManager::Get().Update(deltaTime);

    // Update Audio looping
    AudioManager::Get().UpdateLoopingSounds();

    // Get Menu through Engine (Legacy access)
    auto engine = &Engine::Instance();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    UIManager *uiManager = nullptr;
    Menu *menu = nullptr;

    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
            {
                menu = uiManager->GetMenu();
            }
        }
    }

    // Only handle console toggle here if we are NOT in the menu.
    // When in menu, Menu::HandleKeyboardNavigation handles it to avoid double-toggling.
    if (!m_showMenu && InputManager::Get().IsKeyPressed(KEY_GRAVE) && menu)
    {
        menu->ToggleConsole();
    }

    // Manage cursor visibility based on menu state
    if (m_showMenu)
    {
        // Menu is open - show system cursor
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.MouseDrawCursor = false;

        if (m_cursorDisabled)
        {
            EnableCursor();
            m_cursorDisabled = false;
        }

        HandleMenuActions();
    }
    else
    {
        // Menu is closed - disable keyboard navigation
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

        if (io.NavActive)
        {
            io.NavActive = false;
            io.NavVisible = false;
        }

        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;

        // Game is running
        if (m_isGameInitialized)
        {
            bool consoleOpen =
                menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen();

            if (consoleOpen)
            {
                if (m_cursorDisabled)
                {
                    EnableCursor();
                    m_cursorDisabled = false;
                }
            }
            else
            {
                if (!m_cursorDisabled)
                {
                    DisableCursor();
                    m_cursorDisabled = true;
                }

                // ECS Update Loop
                PlayerSystem::Update(deltaTime);
                MovementSystem::Update(deltaTime);
                CollisionSystem::Update();
                LifetimeSystem::Update(deltaTime);

                // Update Shader Uniforms
                if (m_shaderLoaded && REGISTRY.valid(m_playerEntity))
                {
                    float time = (float)GetTime();
                    SetShaderValue(m_playerShader, m_locTime, &time, SHADER_UNIFORM_FLOAT);

                    auto &velocity = REGISTRY.get<VelocityComponent>(m_playerEntity);
                    float fallSpeed = 0.0f;
                    if (velocity.velocity.y < 0)
                        fallSpeed = std::abs(velocity.velocity.y);

                    SetShaderValue(m_playerShader, m_locFallSpeed, &fallSpeed,
                                   SHADER_UNIFORM_FLOAT);
                }
            }
        }
        else
        {
            if (m_cursorDisabled)
            {
                EnableCursor();
                m_cursorDisabled = false;
            }
        }
    }
}

void GameApplication::OnRender()
{
    // Frame is begun by EngineApplication::Render()

    // Get Menu
    auto *engine = &Engine::Instance();
    // auto moduleManager = engine->GetModuleManager();
    UIManager *uiManager = nullptr;
    Menu *menu = nullptr;

    if (engine->GetModuleManager())
    {
        auto *uiModule = engine->GetModuleManager()->GetModule("UI");
        if (uiModule)
        {
            uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
                menu = uiManager->GetMenu();
        }
    }

    if (m_showMenu && menu)
    {
        rlImGuiBegin();

        // Render the menu
        menu->Render();

        if (menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
        {
            menu->GetConsoleManager()->RenderConsole();
        }
        rlImGuiEnd();
    }
    else
    {
        if (m_isGameInitialized)
        {
            // 3D Rendering
            auto &camera = RenderManager::Get().GetCamera();
            RenderManager::Get().BeginMode3D(camera);

            // Render ECS entities
            RenderSystem::Render();

            // Render Models (ModelLoader)
            if (m_models)
                m_models->DrawAllModels();

            // Render Map Geometry (LevelManager)
            auto levelManager = Engine::Instance().GetService<LevelManager>();
            if (levelManager)
            {
                levelManager->RenderEditorMap();

                // levelManager->RenderSpawnZone(); (only map editor)
            }

            // Render World (Legacy)
            if (m_world)
                m_world->Render();

            RenderManager::Get().EndMode3D();

            // Draw HUD (Chained Together Style)
            if (REGISTRY.valid(m_playerEntity))
            {
                auto &playerComp = REGISTRY.get<PlayerComponent>(m_playerEntity);

                int hours = (int)playerComp.runTimer / 3600;
                int minutes = ((int)playerComp.runTimer % 3600) / 60;
                int seconds = (int)playerComp.runTimer % 60;

                int startX = 20;
                int startY = 80;
                int fontSize = 20;

                // 1. Height Section
                // Vertical Bar
                DrawLineEx({(float)startX, (float)startY - 5}, {(float)startX, (float)startY + 25},
                           2.0f, WHITE);

                // Height Text "134m"
                const char *heightText = TextFormat("%.0fm", playerComp.maxHeight);
                DrawText(heightText, startX + 10, startY, fontSize, WHITE);

                // 2. Timer Section
                // Clock Icon (Circle + Hands)
                int iconX = startX + MeasureText(heightText, fontSize) + 30; // Offset after height
                int iconY = startY + 10;
                int radius = 8;
                DrawCircleLines(iconX, iconY, (float)radius, WHITE);
                DrawLine(iconX, iconY, iconX, iconY - 6, WHITE); // 12 o'clock hand
                DrawLine(iconX, iconY, iconX + 4, iconY, WHITE); // 3 o'clock hand

                // Timer Text "0h 0m 36s"
                const char *timerText = TextFormat("%dh %dm %ds", hours, minutes, seconds);
                DrawText(timerText, iconX + 15, startY, fontSize, WHITE);
            }
        }
    }
    // Render console in game
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu)
        {
            rlImGuiBegin();
            menu->GetConsoleManager()->RenderConsole();
            rlImGuiEnd();
        }
    }

    // Debug Collision
    if (m_showDebugCollision && m_isGameInitialized)
    {
        RenderManager::Get().BeginMode3D(RenderManager::Get().GetCamera());
        CollisionSystem::RenderDebug();
        RenderManager::Get().EndMode3D();
    }

    // Debug Stats
    if (m_showDebugStats)
    {
        DrawFPS(10, 10);

        if (m_isGameInitialized)
        {
            // Draw player position for debugging
            if (REGISTRY.valid(m_playerEntity))
            {
                auto &transform = REGISTRY.get<TransformComponent>(m_playerEntity);
                DrawText(TextFormat("Pos: %.2f, %.2f, %.2f", transform.position.x,
                                    transform.position.y, transform.position.z),
                         10, 30, 20, GREEN);
            }
        }
    }

    // Frame is ended by EngineApplication::Render()
}

void GameApplication::OnShutdown()
{
    TraceLog(LOG_INFO, "[GameApplication] Cleaning up game resources...");

    // Clear ECS
    REGISTRY.clear();

    // Unload player model
    if (m_playerModel.meshes != 0)
    {
        UnloadModel(m_playerModel);
        m_playerModel = {0}; // Prevent double-free
    }

    if (m_shaderLoaded)
    {
        UnloadShader(m_playerShader);
        m_shaderLoaded = false;
    }

    // Shutdown Managers
    RenderManager::Get().Shutdown();
    InputManager::Get().Shutdown();
    AudioManager::Get().Shutdown(); // Optional, destructor handles it

    if (m_collisionManager && !m_collisionManager->GetColliders().empty())
    {
        m_collisionManager->ClearColliders();
        {
            auto *uiModule = Engine::Instance().GetModuleManager()->GetModule("UI");
            if (uiModule)
            {
                auto *uiManager = dynamic_cast<UIManager *>(uiModule);
                if (uiManager && uiManager->GetMenu())
                {
                    uiManager->GetMenu()->SetGameInProgress(false);
                }
            }
        }
    }

    TraceLog(LOG_INFO, "[GameApplication] Game resources cleaned up successfully");
}

void GameApplication::InitInput()
{
    TraceLog(LOG_INFO, "[GameApplication] Setting up game-specific input bindings...");

    auto *engine = &Engine::Instance();
    if (!engine)
    {
        TraceLog(LOG_WARNING, "[GameApplication] No engine provided, skipping input bindings");
        return;
    }

    // Get Menu through UIController
    auto moduleManager = engine->GetModuleManager();
    Menu *menu = nullptr;
    if (moduleManager)
    {
        auto *uiModule = moduleManager->GetModule("UI");
        if (uiModule)
        {
            auto *uiManager = dynamic_cast<UIManager *>(uiModule);
            if (uiManager)
            {
                menu = uiManager->GetMenu();
            }
        }
    }

    if (!menu)
    {
        TraceLog(LOG_WARNING, "[GameApplication] Menu not found, skipping input bindings");
        return;
    }

    engine->GetInputManager()->RegisterAction(
        KEY_F1,
        [this, menu]
        {
            if (!m_showMenu && m_isGameInitialized)
            {
                SaveGameState();
                menu->SetGameInProgress(true);
                m_showMenu = true;
                EnableCursor(); // Show system cursor when opening menu
            }
        });

    engine->GetInputManager()->RegisterAction(
        KEY_ESCAPE,
        [this, menu]
        {
            if (!m_showMenu && m_isGameInitialized)
            {
                SaveGameState();
                menu->ResetAction();
                menu->SetGameInProgress(true);
                m_showMenu = true;
                EnableCursor(); // Show system cursor when opening menu
            }
        });

    engine->GetInputManager()->RegisterAction(KEY_F2,
                                              [this]
                                              {
                                                  m_showDebugCollision = !m_showDebugCollision;
                                                  TraceLog(LOG_INFO, "Debug Collision: %s",
                                                           m_showDebugCollision ? "ON" : "OFF");
                                              });

    engine->GetInputManager()->RegisterAction(KEY_F3,
                                              [this]
                                              {
                                                  m_showDebugStats = !m_showDebugStats;
                                                  TraceLog(LOG_INFO, "Debug Stats: %s",
                                                           m_showDebugStats ? "ON" : "OFF");
                                              });

    TraceLog(LOG_INFO, "[GameApplication] Game input bindings configured.");
}

void GameApplication::HandleMenuActions()
{
    // Get UIManager through ModuleManager
    auto *engine = &Engine::Instance();
    if (!engine)
        return;

    auto moduleManager = engine->GetModuleManager();
    if (!moduleManager)
        return;

    auto *uiModule = moduleManager->GetModule("UI");
    if (!uiModule)
        return;

    auto *uiManager = dynamic_cast<UIManager *>(uiModule);
    if (uiManager)
    {
        uiManager->HandleMenuActions(&m_showMenu, &m_isGameInitialized);
    }
}

void GameApplication::SaveGameState()
{
    if (!m_isGameInitialized || !REGISTRY.valid(m_playerEntity))
    {
        return;
    }

    // Save to simple text file logic
    auto &transform = REGISTRY.get<TransformComponent>(m_playerEntity);
    auto &player = REGISTRY.get<PlayerComponent>(m_playerEntity);

    std::string savePath = std::string(PROJECT_ROOT_DIR) + "/savegame.dat";
    std::ofstream saveFile(savePath);

    if (saveFile.is_open())
    {
        // Format: X Y Z RunTimer MaxHeight
        saveFile << transform.position.x << " " << transform.position.y << " "
                 << transform.position.z << " " << player.runTimer << " " << player.maxHeight;

        saveFile.close();
        TraceLog(LOG_INFO, "[GameApplication] Game Saved: Pos(%.2f, %.2f, %.2f) Time: %.2f",
                 transform.position.x, transform.position.y, transform.position.z, player.runTimer);
    }
    else
    {
        TraceLog(LOG_ERROR, "[GameApplication] Failed to open save file: %s", savePath.c_str());
    }
}
