#include "UIController.h"
#include "../MapSystem/MapSystem.h"
#include "../PlayerSystem/PlayerSystem.h"
#include "core/object/kernel/Core/Kernel.h"
#include "platform/windows/Core/EngineApplication.h"
#include "project/chaineddecos/Menu/Console/ConsoleManager.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include "scene/resources/model/Utils/ModelAnalyzer.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <raylib.h>

UIController::UIController() : m_menu(nullptr), m_kernel(nullptr), m_engine(nullptr)
{
}

UIController::~UIController()
{
    Shutdown();
}

bool UIController::Initialize(Kernel *kernel)
{
    if (!kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[UIController] Initializing...");

    // Get engine dependencies through Kernel
    // Get engine dependencies through Kernel
    auto engineObj = kernel->GetObject<Engine>();

    // Engine not required for Menu, but preferred
    m_engine = engineObj ? engineObj.get() : nullptr;

    if (!m_engine)
    {
        TraceLog(LOG_WARNING,
                 "[UIController] Engine service not found - Menu may have limited functionality");
    }

    // Create our own components
    try
    {
        m_menu = std::make_unique<Menu>();

        // Initialize Menu if Engine is available
        if (m_engine)
        {
            m_menu->Initialize(m_engine);
            m_menu->SetKernel(kernel);

            // Connect AudioManager to SettingsManager
            auto audioManager = kernel->GetService<AudioManager>();
            if (audioManager && m_menu->GetSettingsManager())
            {
                m_menu->GetSettingsManager()->SetAudioManager(audioManager.get());
                TraceLog(LOG_INFO, "[UIController] AudioManager connected to SettingsManager");
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "[UIController] Could not connect AudioManager to SettingsManager");
            }

            // Camera will be injected later in PlayerSystem::RegisterServices
            // after Player is created, so we don't do anything here

            TraceLog(LOG_INFO, "[UIController] Menu initialized");
        }
        else
        {
            TraceLog(LOG_WARNING,
                     "[UIController] Menu created but not fully initialized (no Engine)");
        }

        // Register services in Initialize so they're available to other systems
        RegisterServices(kernel);

        TraceLog(LOG_INFO, "[UIController] Initialized successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIController] Failed to create components: %s", e.what());
        return false;
    }
}

void UIController::Shutdown()
{
    TraceLog(LOG_INFO, "[UIController] Shutting down...");

    // Clean up our own resources (we own them)
    if (m_menu)
    {
        m_menu.reset();
    }

    // Dependencies - references only, don't delete
    m_kernel = nullptr;
    m_engine = nullptr;

    TraceLog(LOG_INFO, "[UIController] Shutdown complete");
}

void UIController::Update(float deltaTime)
{
    if (m_menu)
    {
        m_menu->Update();
    }
    (void)deltaTime;
}

void UIController::Render()
{
    // Menu rendering handled separately by RenderManager
    // This system focuses on logic only
}

void UIController::RegisterServices(Kernel *kernel)
{
    if (!kernel)
    {
        return;
    }

    TraceLog(LOG_INFO, "[UIController] Registering services...");

    // Register our own components as services
    if (m_menu)
    {
        kernel->RegisterService<MenuService>(std::make_shared<MenuService>(m_menu.get()));
        TraceLog(LOG_INFO, "[UIController] MenuService registered");
    }
}

std::vector<std::string> UIController::GetDependencies() const
{
    // No dependencies on other game systems
    return {};
}

ConsoleManager *UIController::GetConsoleManager() const
{
    if (!m_menu)
    {
        return nullptr;
    }
    return m_menu->GetConsoleManager();
}

void UIController::HandleMenuActions(bool *showMenu, bool *isGameInitialized)
{
    if (!m_menu)
    {
        TraceLog(LOG_WARNING, "[UIController] HandleMenuActions() - Menu not available");
        return;
    }

    MenuAction action = m_menu->ConsumeAction();

    switch (action)
    {
    case MenuAction::SinglePlayer:
        HandleSinglePlayer(showMenu, isGameInitialized);
        break;
    case MenuAction::ResumeGame:
        HandleResumeGame(showMenu, isGameInitialized);
        break;
    case MenuAction::StartGameWithMap:
        TraceLog(LOG_INFO,
                 "[UIController] HandleMenuActions() - Starting HandleStartGameWithMap()");
        HandleStartGameWithMap(showMenu, isGameInitialized);
        break;
    case MenuAction::ExitGame:
        HandleExitGame(showMenu);
        break;
    default:
        break;
    }
}

void UIController::HandleSinglePlayer(bool *showMenu, bool *isGameInitialized)
{
    TraceLog(LOG_INFO, "[UIController] HandleSinglePlayer() - Starting singleplayer...");

    if (!m_menu || !m_kernel)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] HandleSinglePlayer() - Required services not available");
        return;
    }

    auto playerSystemService = m_kernel->GetService<PlayerSystemService>();
    if (!playerSystemService || !playerSystemService->playerSystem)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleSinglePlayer() - PlayerSystem not available");
        return;
    }

    m_menu->SetGameInProgress(true);

    // Initialize player after map is loaded
    try
    {
        playerSystemService->playerSystem->InitializePlayer();
        TraceLog(LOG_INFO, "[UIController] HandleSinglePlayer() - Player initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleSinglePlayer() - Failed to initialize player: %s",
                 e.what());
        TraceLog(LOG_WARNING,
                 "[UIController] HandleSinglePlayer() - Player may not render correctly");
    }

    *showMenu = false;         // Hide menu
    *isGameInitialized = true; // Mark game as initialized
}

void UIController::HideMenuAndStartGame(bool *showMenu)
{
    *showMenu = false;

    // Cursor visibility is now managed centrally in GameApplication::OnPostUpdate()
    // based on m_showMenu state

    if (m_menu)
    {
        m_menu->ResetAction();
    }
}

void UIController::EnsurePlayerSafePosition()
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] EnsurePlayerSafePosition() - Kernel not available");
        return;
    }

    auto playerService = m_kernel->GetService<PlayerService>();
    auto collisionManager = m_kernel->GetService<CollisionManager>();

    if (!playerService || !playerService->player || !collisionManager)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] EnsurePlayerSafePosition() - Required services not available");
        return;
    }

    Player *player = playerService->player;

    if (player->GetPlayerPosition().x == 0.0f && player->GetPlayerPosition().y == 0.0f &&
        player->GetPlayerPosition().z == 0.0f)
    {
        TraceLog(LOG_INFO, "[UIController] EnsurePlayerSafePosition() - Player position is origin, "
                           "resetting to safe position");
        player->SetPlayerPosition({0.0f, 2.0f, 0.0f}); // PLAYER_SAFE_SPAWN_HEIGHT = 2.0f
    }

    // Re-setup player collision and movement
    player->GetMovement()->SetCollisionManager(collisionManager.get());
    player->UpdatePlayerBox();
    player->UpdatePlayerCollision();
}

void UIController::ReinitializeCollisionSystemForResume()
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] ReinitializeCollisionSystemForResume() - Kernel not available");
        return;
    }

    auto models = m_kernel->GetService<ModelLoader>();
    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    auto collisionManager = m_kernel->GetService<CollisionManager>();

    if (!mapSystemService || !mapSystemService->mapSystem || !collisionManager || !models)
    {
        TraceLog(LOG_ERROR, "[UIController] ReinitializeCollisionSystemForResume() - Required "
                            "services not available");
        return;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;

    TraceLog(LOG_WARNING, "[UIController] ReinitializeCollisionSystemForResume() - No colliders "
                          "found, reinitializing...");
    std::vector<std::string> requiredModels =
        ModelAnalyzer::GetModelsRequiredForMap(mapSystem->GetCurrentMapPath());

    // Reinitialize collision system safely
    try
    {
        // Clear existing colliders
        collisionManager->ClearColliders();

        // Initialize collision manager
        collisionManager->Initialize();

        // Try to create model collisions, but don't fail if it doesn't work
        try
        {
            collisionManager->CreateAutoCollisionsFromModelsSelective(*models.get(),
                                                                      requiredModels);
            TraceLog(
                LOG_INFO,
                "[UIController] ReinitializeCollisionSystemForResume() - Resume model collisions "
                "created successfully");
        }
        catch (const std::exception &modelCollisionException)
        {
            TraceLog(LOG_WARNING,
                     "[UIController] ReinitializeCollisionSystemForResume() - Resume model "
                     "collision creation "
                     "failed: %s",
                     modelCollisionException.what());
            TraceLog(
                LOG_WARNING,
                "[UIController] ReinitializeCollisionSystemForResume() - Continuing with basic "
                "collision system only");
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] ReinitializeCollisionSystemForResume() - Failed to reinitialize "
                 "collision system "
                 "for resume: %s",
                 e.what());
    }
}

void UIController::HandleResumeGame(bool *showMenu, bool *isGameInitialized)
{
    TraceLog(LOG_INFO, "[UIController] HandleResumeGame() - Resuming game...");

    if (!m_menu || !m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleResumeGame() - Required services not available");
        return;
    }

    auto models = m_kernel->GetService<ModelLoader>();
    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    auto playerSystemService = m_kernel->GetService<PlayerSystemService>();
    auto collisionManager = m_kernel->GetService<CollisionManager>();

    if (!models || !mapSystemService || !mapSystemService->mapSystem || !playerSystemService ||
        !playerSystemService->playerSystem || !collisionManager)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleResumeGame() - Required services not available");
        return;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;
    PlayerSystem *playerSystem = playerSystemService->playerSystem;

    m_menu->SetAction(MenuAction::SinglePlayer);

    // Restore game state first (player position, velocity, etc.)
    playerSystem->RestorePlayerState();
    TraceLog(LOG_INFO, "[UIController] HandleResumeGame() - Game state restored");

    // Ensure game is properly initialized for resume
    if (!(*isGameInitialized))
    {
        TraceLog(LOG_INFO, "[UIController] HandleResumeGame() - Initializing game for resume...");

        // Load models for the current map (use saved map)
        std::vector<std::string> requiredModels =
            ModelAnalyzer::GetModelsRequiredForMap(mapSystem->GetCurrentMapPath());
        models->LoadGameModelsSelective(requiredModels);

        // Initialize basic collision system first
        if (!mapSystem->InitCollisionsWithModelsSafe(requiredModels))
        {
            TraceLog(LOG_ERROR, "[UIController] HandleResumeGame() - Failed to initialize basic "
                                "collision system for singleplayer");
            TraceLog(
                LOG_ERROR,
                "[UIController] HandleResumeGame() - Cannot continue without collision system");
            return;
        }
        TraceLog(
            LOG_INFO,
            "[UIController] HandleResumeGame() - Collision system initialized for singleplayer");

        // Initialize player after map is loaded
        try
        {
            playerSystem->InitializePlayer();
            TraceLog(LOG_INFO, "[UIController] HandleResumeGame() - Player initialized for resume");
        }
        catch (const std::exception &e)
        {
            TraceLog(
                LOG_ERROR,
                "[UIController] HandleResumeGame() - Failed to initialize player for resume: %s",
                e.what());
            TraceLog(LOG_WARNING,
                     "[UIController] HandleResumeGame() - Player may not render correctly");
        }
    }
    else
    {
        // Game is already initialized, just ensure collision system is ready
        if (collisionManager->GetColliders().empty())
        {
            ReinitializeCollisionSystemForResume();
        }

        // Ensure player is properly positioned and set up
        EnsurePlayerSafePosition();
    }

    // Hide the menu and resume the game
    HideMenuAndStartGame(showMenu);
    TraceLog(LOG_INFO, "[UIController] HandleResumeGame() - Game resumed successfully");
}

std::string UIController::ConvertMapNameToPath(const std::string &selectedMapName)
{
    std::string mapPath;
    if (selectedMapName.length() >= 3 && isalpha(selectedMapName[0]) && selectedMapName[1] == ':' &&
        (selectedMapName[2] == '/' || selectedMapName[2] == '\\'))
    {
        // Already an absolute path, use as-is
        mapPath = selectedMapName;
    }
    else
    {
        // For relative paths, construct path using PROJECT_ROOT_DIR and resources/maps/
        // Extract filename to handle cases like "../maporigin.json"
        std::string filename = std::filesystem::path(selectedMapName).filename().string();
        mapPath = PROJECT_ROOT_DIR "/resources/maps/" + filename;
        if (filename.find(".json") == std::string::npos)
        {
            mapPath += ".json";
        }
    }
    return mapPath;
}

std::vector<std::string> UIController::AnalyzeMapForRequiredModels(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "[UIController] AnalyzeMapForRequiredModels() - Analyzing map to determine "
                       "required models...");
    std::vector<std::string> requiredModels;

    try
    {
        requiredModels = ModelAnalyzer::GetModelsRequiredForMap(mapPath);
        if (requiredModels.empty())
        {
            TraceLog(LOG_WARNING, "[UIController] AnalyzeMapForRequiredModels() - No models "
                                  "required for map, but player model is always needed");
            requiredModels.emplace_back("player_low"); // Always include player model
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] AnalyzeMapForRequiredModels() - Failed to analyze map for "
                 "required models: %s",
                 e.what());
        TraceLog(LOG_ERROR, "[UIController] AnalyzeMapForRequiredModels() - Cannot continue "
                            "without model analysis");
        throw;
    }

    TraceLog(LOG_INFO, "[UIController] AnalyzeMapForRequiredModels() - Required models for map:");
    for (const auto &model : requiredModels)
    {
        TraceLog(LOG_INFO, "[UIController] AnalyzeMapForRequiredModels() -   - %s", model.c_str());
    }
    TraceLog(LOG_INFO, "[UIController] AnalyzeMapForRequiredModels() - Total models required: %d",
             requiredModels.size());

    return requiredModels;
}

bool UIController::LoadRequiredModels(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "[UIController] LoadRequiredModels() - Loading required models selectively...");

    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] LoadRequiredModels() - Kernel not available");
        return false;
    }

    auto models = m_kernel->GetService<ModelLoader>();
    if (!models)
    {
        TraceLog(LOG_ERROR, "[UIController] LoadRequiredModels() - ModelLoader not available");
        return false;
    }

    auto loadResult = models->LoadGameModelsSelective(requiredModels);
    if (!loadResult || loadResult->loadedModels == 0)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] LoadRequiredModels() - Failed to load any required models");
        TraceLog(LOG_ERROR, "[UIController] LoadRequiredModels() - Cannot continue without models");
        return false;
    }
    TraceLog(LOG_INFO,
             "[UIController] LoadRequiredModels() - Successfully loaded %d/%d required models in "
             "%.2f seconds",
             loadResult->loadedModels, loadResult->totalModels, loadResult->loadingTime);
    return true;
}

bool UIController::InitializeCollisionSystemWithModels(
    const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO, "[UIController] InitializeCollisionSystemWithModels() - Initializing "
                       "collision system with required models...");

    if (!m_kernel)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] InitializeCollisionSystemWithModels() - Kernel not available");
        return false;
    }

    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    if (!mapSystemService || !mapSystemService->mapSystem)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] InitializeCollisionSystemWithModels() - MapSystem not available");
        return false;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;

    if (!mapSystem->InitCollisionsWithModelsSafe(requiredModels))
    {
        TraceLog(LOG_ERROR, "[UIController] InitializeCollisionSystemWithModels() - Failed to "
                            "initialize collision system with required models");
        TraceLog(LOG_ERROR, "[UIController] InitializeCollisionSystemWithModels() - Cannot "
                            "continue without collision system");
        return false;
    }
    TraceLog(LOG_INFO, "[UIController] InitializeCollisionSystemWithModels() - Collision system "
                       "initialized successfully");
    return true;
}

void UIController::RegisterPreloadedModels()
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] RegisterPreloadedModels() - Kernel not available");
        return;
    }

    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    auto models = m_kernel->GetService<ModelLoader>();

    if (!mapSystemService || !mapSystemService->mapSystem || !models)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] RegisterPreloadedModels() - Required services not available");
        return;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;

    if (!mapSystem->GetGameMap().GetMapModels().empty())
    {
        TraceLog(LOG_INFO,
                 "[UIController] RegisterPreloadedModels() - Registering %d preloaded models from "
                 "map into ModelLoader",
                 mapSystem->GetGameMap().GetMapModels().size());
        for (const auto &p : mapSystem->GetGameMap().GetMapModels())
        {
            const std::string &modelName = p.first;
            const ::Model &loaded = p.second;

            // Validate model before registration
            if (loaded.meshCount > 0)
            {
                if (models->RegisterLoadedModel(modelName, loaded))
                {
                    TraceLog(LOG_INFO,
                             "[UIController] RegisterPreloadedModels() - Successfully registered "
                             "model from map: %s (meshCount: %d)",
                             modelName.c_str(), loaded.meshCount);
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "[UIController] RegisterPreloadedModels() - Failed to register model "
                             "from map: %s",
                             modelName.c_str());
                }
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "[UIController] RegisterPreloadedModels() - Skipping invalid model from "
                         "map: %s (meshCount: %d)",
                         modelName.c_str(), loaded.meshCount);
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "[UIController] RegisterPreloadedModels() - No preloaded models in "
                           "GameMap to register");
    }
}

bool UIController::AutoLoadModelIfNeeded(const std::string &requested, std::string &candidateName)
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] AutoLoadModelIfNeeded() - Kernel not available");
        return false;
    }

    auto models = m_kernel->GetService<ModelLoader>();
    if (!models)
    {
        TraceLog(LOG_ERROR, "[UIController] AutoLoadModelIfNeeded() - ModelLoader not available");
        return false;
    }

    auto available = models->GetAvailableModels();
    bool exists = (std::find(available.begin(), available.end(), requested) != available.end());
    candidateName = requested;

    if (!exists)
    {
        std::string stem = std::filesystem::path(requested).stem().string();
        if (!stem.empty() && std::find(available.begin(), available.end(), stem) != available.end())
        {
            candidateName = stem;
            exists = true;
        }
        else
        {
            std::vector<std::string> exts = {".glb", ".gltf", ".obj"};
            for (const auto &ext : exts)
            {
                std::string resourcePath =
                    std::string(PROJECT_ROOT_DIR) + "/resources/" + requested;
                if (std::filesystem::path(requested).extension().empty())
                    resourcePath = std::string(PROJECT_ROOT_DIR) + "/resources/" + requested + ext;

                TraceLog(LOG_INFO,
                         "[UIController] AutoLoadModelIfNeeded() - Attempting to auto-load model "
                         "'%s' from %s",
                         requested.c_str(), resourcePath.c_str());
                if (models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                {
                    candidateName = stem.empty() ? requested : stem;
                    exists = true;
                    TraceLog(LOG_INFO,
                             "[UIController] AutoLoadModelIfNeeded() - Auto-loaded model '%s'",
                             candidateName.c_str());
                    break;
                }
            }
        }
    }

    return exists;
}

void UIController::CreateModelInstancesForMap()
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] CreateModelInstancesForMap() - Kernel not available");
        return;
    }

    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    auto models = m_kernel->GetService<ModelLoader>();

    if (!mapSystemService || !mapSystemService->mapSystem || !models)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] CreateModelInstancesForMap() - Required services not available");
        return;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;

    TraceLog(LOG_INFO,
             "[UIController] CreateModelInstancesForMap() - Creating model instances for "
             "array-format map (%d objects)",
             mapSystem->GetGameMap().GetMapObjects().size());
    for (const auto &object : mapSystem->GetGameMap().GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string requested = object.modelName;
            std::string candidateName;

            if (!AutoLoadModelIfNeeded(requested, candidateName))
            {
                TraceLog(LOG_WARNING,
                         "[UIController] CreateModelInstancesForMap() - Model '%s' not available "
                         "after auto-load attempts; skipping instance for object '%s'",
                         requested.c_str(), object.name.c_str());
                continue;
            }

            ModelInstanceConfig cfg;
            cfg.position = object.position;
            cfg.rotation = object.rotation;
            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                            ? object.scale.x
                            : 1.0f;
            cfg.color = object.color;
            cfg.spawn = true;

            if (!models->AddInstanceEx(candidateName, cfg))
            {
                TraceLog(
                    LOG_WARNING,
                    "[UIController] CreateModelInstancesForMap() - Failed to add instance for '%s'",
                    candidateName.c_str());
            }
            else
            {
                TraceLog(LOG_INFO,
                         "[UIController] CreateModelInstancesForMap() - Added instance for '%s'",
                         candidateName.c_str());
            }
        }
        else if (object.type == MapObjectType::LIGHT)
        {
            TraceLog(LOG_INFO,
                     "[UIController] CreateModelInstancesForMap() - Skipping LIGHT object '%s' for "
                     "model instance creation",
                     object.name.c_str());
        }
    }
}

void UIController::LoadMapObjects(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "[UIController] LoadMapObjects() - Loading map objects...");

    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[UIController] LoadMapObjects() - Kernel not available");
        throw std::runtime_error("Kernel not available");
    }

    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    if (!mapSystemService || !mapSystemService->mapSystem)
    {
        TraceLog(LOG_ERROR, "[UIController] LoadMapObjects() - MapSystem not available");
        throw std::runtime_error("MapSystem not available");
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;

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
                TraceLog(
                    LOG_INFO,
                    "[UIController] LoadMapObjects() - Detected array format, using LoadGameMap");

                MapLoader loader;
                mapSystem->GetGameMap() = loader.LoadMap(mapPath.c_str());

                // Register any models that MapLoader preloaded into the GameMap
                RegisterPreloadedModels();

                // Create model instances
                CreateModelInstancesForMap();
            }
            else
            {
                TraceLog(LOG_INFO, "[UIController] LoadMapObjects() - Detected editor format, "
                                   "using LoadEditorMap");
                mapSystem->LoadEditorMap(mapPath);
            }
        }
        else
        {
            TraceLog(LOG_ERROR, "[UIController] LoadMapObjects() - Cannot open map file: %s",
                     mapPath.c_str());
            throw std::runtime_error("Cannot open map file");
        }

        TraceLog(LOG_INFO,
                 "[UIController] LoadMapObjects() - Map loaded successfully with %d objects",
                 mapSystem->GetGameMap().GetMapObjects().size());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIController] LoadMapObjects() - Failed to load map: %s", e.what());
        TraceLog(LOG_ERROR, "[UIController] LoadMapObjects() - Cannot continue without map");
        throw;
    }
}

void UIController::HandleStartGameWithMap(bool *showMenu, bool *isGameInitialized)
{
    TraceLog(LOG_INFO,
             "[UIController] HandleStartGameWithMap() - Starting game with selected map...");

    if (!m_menu || !m_kernel)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] HandleStartGameWithMap() - Required services not available");
        return;
    }

    // Verify all required services are available
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Getting services...");
    auto mapSystemService = m_kernel->GetService<MapSystemService>();
    auto models = m_kernel->GetService<ModelLoader>();
    auto playerSystemService = m_kernel->GetService<PlayerSystemService>();

    if (!mapSystemService || !mapSystemService->mapSystem)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleStartGameWithMap() - MapSystem not available");
        return;
    }

    if (!models)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleStartGameWithMap() - ModelLoader not available");
        return;
    }

    if (!playerSystemService || !playerSystemService->playerSystem)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleStartGameWithMap() - PlayerSystem not available");
        return;
    }

    MapSystem *mapSystem = mapSystemService->mapSystem;
    PlayerSystem *playerSystem = playerSystemService->playerSystem;

    TraceLog(LOG_INFO,
             "[UIController] HandleStartGameWithMap() - All services available, proceeding...");
    m_menu->SetGameInProgress(true);
    std::string selectedMapName = m_menu->GetSelectedMapName();
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Selected map: %s",
             selectedMapName.c_str());

    // Convert map name to full path
    std::string mapPath = ConvertMapNameToPath(selectedMapName);
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Full map path: %s",
             mapPath.c_str());

    // Step 1: Analyze map to determine required models
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 1: Analyzing map...");
    std::vector<std::string> requiredModels;
    try
    {
        requiredModels = AnalyzeMapForRequiredModels(mapPath);
        TraceLog(
            LOG_INFO,
            "[UIController] HandleStartGameWithMap() - Step 1 complete: Found %d required models",
            requiredModels.size());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleStartGameWithMap() - Failed to analyze map: %s",
                 e.what());
        return;
    }

    // Step 2: Load only the required models selectively
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 2: Loading models...");
    if (!LoadRequiredModels(requiredModels))
    {
        TraceLog(LOG_ERROR,
                 "[UIController] HandleStartGameWithMap() - Failed to load required models");
        return;
    }
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 2 complete: Models loaded");

    // Step 3: Initialize collision system with required models
    TraceLog(LOG_INFO,
             "[UIController] HandleStartGameWithMap() - Step 3: Initializing collision system...");
    if (!InitializeCollisionSystemWithModels(requiredModels))
    {
        TraceLog(LOG_ERROR,
                 "[UIController] HandleStartGameWithMap() - Failed to initialize collision system");
        return;
    }
    TraceLog(
        LOG_INFO,
        "[UIController] HandleStartGameWithMap() - Step 3 complete: Collision system initialized");

    // Step 4: Load the map objects
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 4: Loading map objects...");
    try
    {
        LoadMapObjects(mapPath);
        TraceLog(LOG_INFO,
                 "[UIController] HandleStartGameWithMap() - Step 4 complete: Map objects loaded");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIController] HandleStartGameWithMap() - Failed to load map: %s",
                 e.what());
        return;
    }

    // Step 5: Initialize player after map is loaded
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 5: Initializing player...");

    try
    {
        playerSystem->InitializePlayer();
        TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Step 5 complete: Player "
                           "initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIController] HandleStartGameWithMap() - Failed to initialize player: %s",
                 e.what());
        TraceLog(LOG_WARNING,
                 "[UIController] HandleStartGameWithMap() - Player may not render correctly");
    }

    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Game initialization complete");
    *isGameInitialized = true;

    // Hide menu and start the game
    TraceLog(LOG_INFO,
             "[UIController] HandleStartGameWithMap() - Hiding menu and starting game...");
    HideMenuAndStartGame(showMenu);
    TraceLog(LOG_INFO, "[UIController] HandleStartGameWithMap() - Complete!");
}

void UIController::HandleExitGame(bool *showMenu)
{
    TraceLog(LOG_INFO, "[UIController] HandleExitGame() - Exit game requested from menu.");

    if (m_menu)
    {
        // Clear game state when exiting
        m_menu->SetGameInProgress(false);
        m_menu->ResetAction();
    }

    *showMenu = true; // Show menu one last time before exit

    if (m_engine)
    {
        m_engine->RequestExit();
    }
}
