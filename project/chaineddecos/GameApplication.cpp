#include "GameApplication.h"
#include "GameLayer.h"
#include "core/application/EngineApplication.h"
#include "scene/main/core/LevelManager.h"

// #include "systems/playersystem/playerController.h"
// #include "systems/renderingsystem/RenderingSystem.h"

#include "components/input/core/InputManager.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/config/ConfigManager.h"
#include "core/ecs/Examples.h"
#include "core/ecs/components.h"
#include "core/gui/components/GuiButton.h"
#include "core/module/ModuleManager.h"
#include "project/chaineddecos/gamegui/Menu.h"
#include "project/chaineddecos/player/core/Player.h"
#include "scene/main/core/LevelManager.h"
#include "scene/resources/model/core/Model.h"
#include <raylib.h>
#include <rlImGui.h>

using ChainedDecos::InputManager;
using ChainedDecos::MenuEvent;
using ChainedDecos::MenuEventType;
// MenuEventCallback is inside Menu class
using MenuEventCallback = Menu::MenuEventCallback;

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
    auto &engine = Engine::Instance();
    // 2. Register Game Systems (Modules)
    engine.RegisterModule(std::make_unique<LevelManager>());
    // UIManager removed - replaced by direct Menu management in GameApplication

    // PlayerController and RenderingSystem are replaced by ECS Systems
    // engine.RegisterModule(std::make_unique<PlayerController>());
    // engine.RegisterModule(std::make_unique<RenderingSystem>());

    TraceLog(LOG_INFO, "[GameApplication] Game systems registered.");
}

void GameApplication::OnStart()
{
    TraceLog(LOG_INFO, "[GameApplication] Starting game...");

    // Initialize Static Singletons
    // Note: RenderManager/InputManager/AudioManager are already initialized by CoreServices

    AudioManager::Get().LoadSound("player_fall", std::string(PROJECT_ROOT_DIR) +
                                                     "/resources/audio/wind-gust_fall.wav");

    // Initialize Menu
    m_menu = std::make_unique<Menu>();
    m_menu->Initialize(&Engine::Instance());
    m_menu->SetupStyle();

    // Register Menu Events
    m_menu->SetEventCallback(
        [this](const ChainedDecos::MenuEvent &event)
        {
            switch (event.GetMenuEventType())
            {
            case ChainedDecos::MenuEventType::StartGame:
            case ChainedDecos::MenuEventType::StartGameWithMap:
            {
                std::string mapName = event.GetMapName();
                if (mapName.empty())
                    mapName = m_menu->GetSelectedMapName();

                auto levelManager = Engine::Instance().GetService<ILevelManager>();
                if (levelManager && levelManager->LoadScene(mapName))
                {
                    m_isGameInitialized = true;
                    m_showMenu = false;
                }
                break;
            }
            case ChainedDecos::MenuEventType::ResumeGame:
            {
                if (m_isGameInitialized)
                {
                    m_showMenu = false;
                }
                break;
            }
            case ChainedDecos::MenuEventType::ExitGame:
            {
                Engine::Instance().RequestExit();
                break;
            }
            case ChainedDecos::MenuEventType::BackToMain:
            {
                // Internal menu state change handled by Menu class
                break;
            }
            default:
                break;
            }
        });

    TraceLog(LOG_INFO, "[GameApplication] Menu initialized and events registered");

    // Initialize ECS
    REGISTRY.clear();

    // Explicitly load player model
    auto models = Engine::Instance().GetService<ModelLoader>();
    if (models)
    {
        std::string playerModelPath = std::string(PROJECT_ROOT_DIR) + "/resources/player_low.glb";
        if (models->LoadSingleModel("player_low", playerModelPath))
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
    if (models)
    {
        // "player_low" is loaded by LevelManager/ModelLoader according to logs
        auto modelOpt = models->GetModelByName("player_low");
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

            // Assign shader to model logic moved to after model initialization/fallback
        }
        else
        {
            TraceLog(LOG_WARNING, "[GameApplication] Failed to load player_effect shader");
        }
    }

    // Initialize player state
    Vector3 spawnPos = {0, 2, 0};

    // Load HUD Font
    std::string fontPath =
        std::string(PROJECT_ROOT_DIR) + "/resources/font/gantari/static/gantari-Bold.ttf";

    // Load font with higher size for better quality scaling
    m_hudFont = LoadFontEx(fontPath.c_str(), 96, 0, 0);

    if (m_hudFont.baseSize > 0)
    {
        SetTextureFilter(m_hudFont.texture, TEXTURE_FILTER_BILINEAR);
        m_fontLoaded = true;
        TraceLog(LOG_INFO, "[GameApplication] Loaded HUD font: %s", fontPath.c_str());
    }
    else
    {
        TraceLog(LOG_ERROR, "[GameApplication] Failed to load HUD font: %s. Loading default.",
                 fontPath.c_str());
        m_fontLoaded = false;
        m_hudFont = GetFontDefault();
    }
    // Push GameLayer using the new Layer system (Cherno-inspired)
    if (GetAppRunner())
    {
        GetAppRunner()->PushLayer(new GameLayer());
    }

    // Load mouse sensitivity from config
    ConfigManager configManager;
    configManager.LoadFromFile(std::string(PROJECT_ROOT_DIR) + "/game.cfg");
    float sensitivity = configManager.GetMouseSensitivity();
    if (sensitivity <= 0.0f)
        sensitivity = 0.15f; // Default if not set

    if (playerModelPtr)
    {
        TraceLog(LOG_INFO, "[GameApplication] Using existing model 'player_low'");
        m_playerEntity =
            ECSExamples::CreatePlayer(spawnPos, playerModelPtr, 8.0f, 12.0f, sensitivity);
    }
    else
    {
        TraceLog(LOG_WARNING, "[GameApplication] 'player_low' not found, using default cube.");
        m_playerModel = LoadModelFromMesh(GenMeshCube(0.8f, 1.8f, 0.8f));
        m_playerEntity =
            ECSExamples::CreatePlayer(spawnPos, &m_playerModel, 8.0f, 12.0f, sensitivity);
    }

    // Apply shader to final player model (either loaded or fallback)
    if (m_shaderLoaded)
    {
        Model *finalModel = playerModelPtr ? playerModelPtr : &m_playerModel;
        if (finalModel->materials != nullptr && finalModel->materialCount > 0)
        {
            finalModel->materials[0].shader = m_playerShader;
            TraceLog(LOG_INFO,
                     "[GameApplication] Applied player_effect shader to final player model");
        }
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

    // Initial state - show menu (unless skipMenu is set)
    m_showMenu = !m_gameConfig.skipMenu;

    // Load initial map if provided
    if (!m_gameConfig.mapPath.empty())
    {
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager && levelManager->LoadScene(m_gameConfig.mapPath))
        {
            m_isGameInitialized = true;
            // If map is loaded from command line, we might want to skip menu
            if (m_gameConfig.skipMenu)
                m_showMenu = false;
        }
    }

    // Initialize cursor state
    m_cursorDisabled = !m_showMenu;
    if (m_cursorDisabled)
        DisableCursor();
    else
        EnableCursor();

    // Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.MouseDrawCursor = false;

    // Initialize input
    InitInput();

    // Game is initialized only if a map was loaded
    if (!m_isGameInitialized)
    {
        m_isGameInitialized = false;
    }

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

    Menu *menu = m_menu.get();

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

        // Menu actions are now handled via event callbacks registered in OnStart()
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

                // ECS Systems are now handled by GameLayer

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
    Menu *menu = m_menu.get();

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

            // Render ECS entities - Now handled by GameLayer
            // RenderSystem::Render();

            // Render Models (ModelLoader)
            auto models = Engine::Instance().GetService<ModelLoader>();
            if (models)
                models->DrawAllModels();

            // Render Map Geometry (LevelManager)
            auto levelManager = Engine::Instance().GetService<ILevelManager>();
            if (levelManager)
            {
                levelManager->RenderEditorMap();

                // levelManager->RenderSpawnZone(); (only map editor)
            }

            // Render World (Legacy)
            auto world = Engine::Instance().GetService<WorldManager>();
            if (world)
                world->Render();

            RenderManager::Get().EndMode3D();

            // Draw HUD (Chained Together Style)
            if (REGISTRY.valid(m_playerEntity))
            {
                auto &playerComp = REGISTRY.get<PlayerComponent>(m_playerEntity);

                int hours = (int)playerComp.runTimer / 3600;
                int minutes = ((int)playerComp.runTimer % 3600) / 60;
                int seconds = (int)playerComp.runTimer % 60;

                int startX = 40;
                int startY = 80;
                // float fontSize = 32.0f; // Unused
                float spacing = 2.0f; // Font spacing

                Font *fontToUse = m_fontLoaded ? &m_hudFont : nullptr;

                // 1. Height Section
                // Height Text "height : 134m"
                const char *heightText = TextFormat("height : %.0fm", playerComp.maxHeight);
                float fontSizeHeight = 36.0f;
                Vector2 heightSize;

                if (m_fontLoaded)
                    heightSize = MeasureTextEx(m_hudFont, heightText, fontSizeHeight, spacing);
                else
                    heightSize = {(float)MeasureText(heightText, (int)fontSizeHeight),
                                  fontSizeHeight};

                // Draw Height Shadow
                Vector2 heightPos = {(float)startX, (float)startY};
                Vector2 shadowOffset = {2.0f, 2.0f};

                if (m_fontLoaded)
                    DrawTextEx(m_hudFont, heightText,
                               {heightPos.x + shadowOffset.x, heightPos.y + shadowOffset.y},
                               fontSizeHeight, spacing, ColorAlpha(BLACK, 0.5f));
                else
                    DrawText(heightText, (int)(heightPos.x + shadowOffset.x),
                             (int)(heightPos.y + shadowOffset.y), (int)fontSizeHeight, BLACK);

                // Draw Height Text
                if (m_fontLoaded)
                    DrawTextEx(m_hudFont, heightText, heightPos, fontSizeHeight, spacing, WHITE);
                else
                    DrawText(heightText, (int)heightPos.x, (int)heightPos.y, (int)fontSizeHeight,
                             WHITE);

                // Vertical separator bar
                int barX = (int)(heightPos.x + heightSize.x + 10);
                DrawLineEx({(float)barX, (float)startY}, {(float)barX, (float)startY + 30}, 3.0f,
                           WHITE);

                // 2. Timer Section with Clock Icon
                // Timer Text - Format "MM:SS" or "HH:MM:SS"
                const char *timerText;
                if (hours > 0)
                    timerText = TextFormat("%02d:%02d:%02d", hours, minutes, seconds);
                else
                    timerText = TextFormat("%02d:%02d", minutes, seconds);

                // Clock icon position
                int iconX = (int)(heightPos.x + heightSize.x + 20);
                int iconY = (int)(startY + 12); // Centered with text
                int iconRadius = 10;

                // Draw clock circle with shadow
                DrawCircle(iconX + 1, iconY + 1, (float)iconRadius, ColorAlpha(BLACK, 0.3f));
                DrawCircle(iconX, iconY, (float)iconRadius, WHITE);
                DrawCircle(iconX, iconY, (float)iconRadius - 1, ColorAlpha(SKYBLUE, 0.2f));

                // Clock hands
                DrawLine(iconX, iconY, iconX, iconY - 6, BLACK); // Hour hand
                DrawLine(iconX, iconY, iconX + 5, iconY, BLACK); // Minute hand
                DrawCircle(iconX, iconY, 2.0f, BLACK);           // Center dot

                // Timer text position
                int timerX = iconX + iconRadius + 8;
                float fontSizeTimer = 28.0f; // Slightly larger

                // Draw Timer Shadow
                if (m_fontLoaded)
                    DrawTextEx(m_hudFont, timerText,
                               {(float)timerX + shadowOffset.x, (float)startY + shadowOffset.y},
                               fontSizeTimer, spacing, ColorAlpha(BLACK, 0.5f));
                else
                    DrawText(timerText, timerX + 2, startY + 2, (int)fontSizeTimer, BLACK);

                // Draw Timer Text
                if (m_fontLoaded)
                    DrawTextEx(m_hudFont, timerText, {(float)timerX, (float)startY}, fontSizeTimer,
                               spacing, WHITE);
                else
                    DrawText(timerText, timerX, startY, (int)fontSizeTimer, WHITE);
            }
        }
    }
    // Render console in game - Now handled inside menu->Render() when showMenu is true
    // If we want it while playing, we need a single rlImGuiBegin/End block per frame
    if (menu && menu->GetConsoleManager() && menu->GetConsoleManager()->IsConsoleOpen())
    {
        if (!m_showMenu)
        {
            rlImGuiBegin();
            menu->Render(); // Calling Render here instead of just Console so screens can handle it
                            // if needed
            rlImGuiEnd();
        }
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

    if (m_fontLoaded)
    {
        UnloadFont(m_hudFont);
        m_fontLoaded = false;
    }

    // Shutdown Managers
    RenderManager::Get().Shutdown();
    InputManager::Get().Shutdown();
    AudioManager::Get().Shutdown(); // Optional, destructor handles it

    auto collisionManager = Engine::Instance().GetService<CollisionManager>();
    if (collisionManager && !collisionManager->GetColliders().empty())
    {
        collisionManager->ClearColliders();
        {
            if (m_menu)
            {
                m_menu->SetGameInProgress(false);
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

    if (!m_menu)
    {
        TraceLog(LOG_WARNING, "[GameApplication] Menu not found, skipping input bindings");
        return;
    }

    auto *menu = m_menu.get();

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
                m_menu->SetGameInProgress(true);
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

// HandleMenuActions removed - replaced by event callbacks
