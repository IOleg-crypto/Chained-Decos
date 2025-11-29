#include "UIManager.h"
#include "../../../core/engine/Engine.h"
#include "../../../core/engine/EngineApplication.h"
#include "../../Menu/Console/ConsoleManager.h"
#include "../../Menu/Menu.h"
#include "../../Player/Core/Player.h"
#include "../MapSystem/LevelManager.h"
#include "../PlayerSystem/PlayerController.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include "scene/resources/model/Utils/ModelAnalyzer.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include <algorithm>
#include <fstream>
#include <raylib.h>

UIManager::UIManager() : m_menu(nullptr), m_engine(nullptr)
{
}

UIManager::~UIManager()
{
    Shutdown();
}

bool UIManager::Initialize(Engine *engine)
{
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] Engine is null");
        return false;
    }

    m_engine = engine;
    TraceLog(LOG_INFO, "[UIManager] Initializing...");

    // Create our own components
    try
    {
        m_menu = std::make_unique<Menu>();

        // Initialize Menu if Engine is available
        if (m_engine)
        {
            m_menu->Initialize(m_engine);
            // m_menu->SetKernel(kernel); // Removed

            // Connect AudioManager to SettingsManager
            auto audioManager = engine->GetService<AudioManager>();
            if (audioManager && m_menu->GetSettingsManager())
            {
                m_menu->GetSettingsManager()->SetAudioManager(audioManager.get());
                TraceLog(LOG_INFO, "[UIManager] AudioManager connected to SettingsManager");
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "[UIManager] Could not connect AudioManager to SettingsManager");
            }

            // Camera will be injected later in PlayerSystem::RegisterServices
            // after Player is created, so we don't do anything here

            TraceLog(LOG_INFO, "[UIManager] Menu initialized");
        }
        else
        {
            TraceLog(LOG_WARNING, "[UIManager] Menu created but not fully initialized (no Engine)");
        }

        // Register services in Initialize so they're available to other systems
        RegisterServices(engine);

        TraceLog(LOG_INFO, "[UIManager] Initialized successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIManager] Failed to create components: %s", e.what());
        return false;
    }
}

void UIManager::Shutdown()
{
    TraceLog(LOG_INFO, "[UIManager] Shutting down...");

    // Clean up our own resources (we own them)
    if (m_menu)
    {
        m_menu.reset();
    }

    // Dependencies - references only, don't delete
    m_engine = nullptr;

    TraceLog(LOG_INFO, "[UIManager] Shutdown complete");
}

void UIManager::Update(float deltaTime)
{
    if (m_menu)
    {
        m_menu->Update();
    }
    (void)deltaTime;
}

void UIManager::Render()
{
    // Menu rendering handled separately by RenderManager
    // This system focuses on logic only
}

void UIManager::RegisterServices(Engine *engine)
{
    if (!engine)
    {
        return;
    }

    TraceLog(LOG_INFO, "[UIManager] Registering services...");

    // Register our own components as services
    if (m_menu)
    {
        // Menu is accessed via UIManager, no need to register separate service
        TraceLog(LOG_INFO, "[UIManager] Menu initialized");
    }
}

std::vector<std::string> UIManager::GetDependencies() const
{
    // No dependencies on other game systems
    return {};
}

ConsoleManager *UIManager::GetConsoleManager() const
{
    if (!m_menu)
    {
        return nullptr;
    }
    return m_menu->GetConsoleManager();
}

void UIManager::HandleMenuActions(bool *showMenu, bool *isGameInitialized)
{
    if (!m_menu)
    {
        TraceLog(LOG_WARNING, "[UIManager] HandleMenuActions() - Menu not available");
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
        TraceLog(LOG_INFO, "[UIManager] HandleMenuActions() - Starting HandleStartGameWithMap()");
        HandleStartGameWithMap(showMenu, isGameInitialized);
        break;
    case MenuAction::ExitGame:
        HandleExitGame(showMenu);
        break;
    default:
        break;
    }
}

void UIManager::HandleSinglePlayer(bool *showMenu, bool *isGameInitialized)
{
    TraceLog(LOG_INFO, "[UIManager] HandleSinglePlayer() - Starting singleplayer...");

    if (!m_menu || !m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] HandleSinglePlayer() - Required services not available");
        return;
    }

    // Get PlayerController through ModuleManager
    PlayerController *playerController = nullptr;
    if (m_engine->GetModuleManager())
    {
        auto *module = m_engine->GetModuleManager()->GetModule("Player");
        if (module)
        {
            playerController = dynamic_cast<PlayerController *>(module);
        }
    }

    if (!playerController)
    {
        TraceLog(LOG_ERROR, "[UIManager] HandleSinglePlayer() - PlayerController not available");
        return;
    }

    m_menu->SetGameInProgress(true);

    // Initialize player after map is loaded
    try
    {
        playerController->InitializePlayer();
        TraceLog(LOG_INFO, "[UIManager] HandleSinglePlayer() - Player initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIManager] HandleSinglePlayer() - Failed to initialize player: %s",
                 e.what());
        TraceLog(LOG_WARNING, "[UIManager] HandleSinglePlayer() - Player may not render correctly");
    }

    *showMenu = false;         // Hide menu
    *isGameInitialized = true; // Mark game as initialized
}

void UIManager::HideMenuAndStartGame(bool *showMenu)
{
    *showMenu = false;

    // Cursor visibility is now managed centrally in GameApplication::OnPostUpdate()
    // based on m_showMenu state

    if (m_menu)
    {
        m_menu->ResetAction();
    }
}

void UIManager::EnsurePlayerSafePosition()
{
    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] EnsurePlayerSafePosition() - Engine not available");
        return;
    }

    auto player = m_engine->GetPlayer();
    auto collisionManager = m_engine->GetService<CollisionManager>();

    if (!player || !collisionManager)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] EnsurePlayerSafePosition() - Required services not available");
        return;
    }

    if (player->GetPlayerPosition().x == 0.0f && player->GetPlayerPosition().y == 0.0f &&
        player->GetPlayerPosition().z == 0.0f)
    {
        TraceLog(LOG_INFO, "[UIManager] EnsurePlayerSafePosition() - Player position is origin, "
                           "resetting to safe position");
        player->SetPlayerPosition({0.0f, 2.0f, 0.0f}); // PLAYER_SAFE_SPAWN_HEIGHT = 2.0f
    }

    // Re-setup player collision and movement
    player->GetMovement()->SetCollisionManager(collisionManager.get());
    player->UpdatePlayerBox();
    player->UpdatePlayerCollision();
}

void UIManager::ReinitializeCollisionSystemForResume()
{
    if (!m_engine)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] ReinitializeCollisionSystemForResume() - Engine not available");
        return;
    }

    auto models = m_engine->GetService<ModelLoader>();
    auto levelManager = m_engine->GetLevelManager();
    auto collisionManager = m_engine->GetService<CollisionManager>();

    if (!levelManager || !collisionManager || !models)
    {
        TraceLog(LOG_ERROR, "[UIManager] ReinitializeCollisionSystemForResume() - Required "
                            "services not available");
        return;
    }

    TraceLog(LOG_WARNING, "[UIManager] ReinitializeCollisionSystemForResume() - No colliders "
                          "found, reinitializing...");
    std::vector<std::string> requiredModels =
        ModelAnalyzer::GetModelsRequiredForMap(levelManager->GetCurrentMapPath());

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
            TraceLog(LOG_INFO,
                     "[UIManager] ReinitializeCollisionSystemForResume() - Resume model collisions "
                     "created successfully");
        }
        catch (const std::exception &modelCollisionException)
        {
            TraceLog(LOG_WARNING,
                     "[UIManager] ReinitializeCollisionSystemForResume() - Resume model "
                     "collision creation "
                     "failed: %s",
                     modelCollisionException.what());
            TraceLog(LOG_WARNING,
                     "[UIManager] ReinitializeCollisionSystemForResume() - Continuing with basic "
                     "collision system only");
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] ReinitializeCollisionSystemForResume() - Failed to reinitialize "
                 "collision system "
                 "for resume: %s",
                 e.what());
    }
}

void UIManager::HandleResumeGame(bool *showMenu, bool *isGameInitialized)
{
    TraceLog(LOG_INFO, "[UIManager] HandleResumeGame() - Resuming game...");

    if (!m_menu || !m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] HandleResumeGame() - Required services not available");
        return;
    }

    auto models = m_engine->GetService<ModelLoader>();
    auto levelManager = m_engine->GetLevelManager();
    auto collisionManager = m_engine->GetService<CollisionManager>();

    // Get PlayerController through ModuleManager
    PlayerController *playerController = nullptr;
    if (m_engine->GetModuleManager())
    {
        auto *module = m_engine->GetModuleManager()->GetModule("Player");
        if (module)
        {
            playerController = dynamic_cast<PlayerController *>(module);
        }
    }

    if (!models || !levelManager || !playerController || !collisionManager)
    {
        TraceLog(LOG_ERROR, "[UIManager] HandleResumeGame() - Required services not available");
        return;
    }

    m_menu->SetAction(MenuAction::SinglePlayer);

    // Restore game state first (player position, velocity, etc.)
    playerController->RestorePlayerState();
    TraceLog(LOG_INFO, "[UIManager] HandleResumeGame() - Game state restored");

    // Ensure game is properly initialized for resume
    if (!(*isGameInitialized))
    {
        TraceLog(LOG_INFO, "[UIManager] HandleResumeGame() - Initializing game for resume...");

        // Load models for the current map (use saved map)
        std::vector<std::string> requiredModels =
            ModelAnalyzer::GetModelsRequiredForMap(levelManager->GetCurrentMapPath());
        models->LoadGameModelsSelective(requiredModels);

        // Initialize basic collision system first
        if (!levelManager->InitCollisionsWithModelsSafe(requiredModels))
        {
            TraceLog(LOG_ERROR, "[UIManager] HandleResumeGame() - Failed to initialize basic "
                                "collision system for singleplayer");
            TraceLog(LOG_ERROR,
                     "[UIManager] HandleResumeGame() - Cannot continue without collision system");
            return;
        }
        TraceLog(LOG_INFO,
                 "[UIManager] HandleResumeGame() - Collision system initialized for singleplayer");

        // Initialize player after map is loaded
        try
        {
            playerController->InitializePlayer();
            TraceLog(LOG_INFO, "[UIManager] HandleResumeGame() - Player initialized for resume");
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR,
                     "[UIManager] HandleResumeGame() - Failed to initialize player for resume: %s",
                     e.what());
            TraceLog(LOG_WARNING,
                     "[UIManager] HandleResumeGame() - Player may not render correctly");
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
    TraceLog(LOG_INFO, "[UIManager] HandleResumeGame() - Game resumed successfully");
}

std::string UIManager::ConvertMapNameToPath(const std::string &selectedMapName)
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

std::vector<std::string> UIManager::AnalyzeMapForRequiredModels(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "[UIManager] AnalyzeMapForRequiredModels() - Analyzing map to determine "
                       "required models...");
    std::vector<std::string> requiredModels;

    try
    {
        requiredModels = ModelAnalyzer::GetModelsRequiredForMap(mapPath);
        if (requiredModels.empty())
        {
            TraceLog(LOG_WARNING, "[UIManager] AnalyzeMapForRequiredModels() - No models "
                                  "required for map, but player model is always needed");
            requiredModels.emplace_back("player_low"); // Always include player model
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] AnalyzeMapForRequiredModels() - Failed to analyze map for "
                 "required models: %s",
                 e.what());
        TraceLog(LOG_ERROR, "[UIManager] AnalyzeMapForRequiredModels() - Cannot continue "
                            "without model analysis");
        throw;
    }

    TraceLog(LOG_INFO, "[UIManager] AnalyzeMapForRequiredModels() - Required models for map:");
    for (const auto &model : requiredModels)
    {
        TraceLog(LOG_INFO, "[UIManager] AnalyzeMapForRequiredModels() -   - %s", model.c_str());
    }
    TraceLog(LOG_INFO, "[UIManager] AnalyzeMapForRequiredModels() - Total models required: %d",
             requiredModels.size());

    return requiredModels;
}

bool UIManager::LoadRequiredModels(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO, "[UIManager] LoadRequiredModels() - Loading required models selectively...");

    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] LoadRequiredModels() - Engine not available");
        return false;
    }

    auto models = m_engine->GetService<ModelLoader>();
    if (!models)
    {
        TraceLog(LOG_ERROR, "[UIManager] LoadRequiredModels() - ModelLoader not available");
        return false;
    }

    auto loadResult = models->LoadGameModelsSelective(requiredModels);
    if (!loadResult || loadResult->loadedModels == 0)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] LoadRequiredModels() - Failed to load any required models");
        TraceLog(LOG_ERROR, "[UIManager] LoadRequiredModels() - Cannot continue without models");
        return false;
    }
    TraceLog(LOG_INFO,
             "[UIManager] LoadRequiredModels() - Successfully loaded %d/%d required models in "
             "%.2f seconds",
             loadResult->loadedModels, loadResult->totalModels, loadResult->loadingTime);
    return true;
}

bool UIManager::InitializeCollisionSystemWithModels(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO, "[UIManager] InitializeCollisionSystemWithModels() - Initializing "
                       "collision system with required models...");

    if (!m_engine)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] InitializeCollisionSystemWithModels() - Engine not available");
        return false;
    }

    auto levelManager = m_engine->GetLevelManager();
    if (!levelManager)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] InitializeCollisionSystemWithModels() - LevelManager not available");
        return false;
    }

    if (!levelManager->InitCollisionsWithModelsSafe(requiredModels))
    {
        TraceLog(LOG_ERROR, "[UIManager] InitializeCollisionSystemWithModels() - Failed to "
                            "initialize collision system with required models");
        TraceLog(LOG_ERROR, "[UIManager] InitializeCollisionSystemWithModels() - Cannot "
                            "continue without collision system");
        return false;
    }
    TraceLog(LOG_INFO, "[UIManager] InitializeCollisionSystemWithModels() - Collision system "
                       "initialized successfully");
    return true;
}

void UIManager::RegisterPreloadedModels()
{
    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] RegisterPreloadedModels() - Engine not available");
        return;
    }

    auto levelManager = m_engine->GetLevelManager();
    auto models = m_engine->GetService<ModelLoader>();

    if (!levelManager || !models)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] RegisterPreloadedModels() - Required services not available");
        return;
    }

    if (!levelManager->GetGameMap().GetMapModels().empty())
    {
        TraceLog(LOG_INFO,
                 "[UIManager] RegisterPreloadedModels() - Registering %d preloaded models from "
                 "map into ModelLoader",
                 levelManager->GetGameMap().GetMapModels().size());
        for (const auto &p : levelManager->GetGameMap().GetMapModels())
        {
            const std::string &modelName = p.first;
            const ::Model &loaded = p.second;

            // Validate model before registration
            if (loaded.meshCount > 0)
            {
                if (models->RegisterLoadedModel(modelName, loaded))
                {
                    TraceLog(LOG_INFO,
                             "[UIManager] RegisterPreloadedModels() - Successfully registered "
                             "model from map: %s (meshCount: %d)",
                             modelName.c_str(), loaded.meshCount);
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "[UIManager] RegisterPreloadedModels() - Failed to register model "
                             "from map: %s",
                             modelName.c_str());
                }
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "[UIManager] RegisterPreloadedModels() - Skipping invalid model from "
                         "map: %s (meshCount: %d)",
                         modelName.c_str(), loaded.meshCount);
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "[UIManager] RegisterPreloadedModels() - No preloaded models in "
                           "GameMap to register");
    }
}

bool UIManager::AutoLoadModelIfNeeded(const std::string &requested, std::string &candidateName)
{
    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] AutoLoadModelIfNeeded() - Engine not available");
        return false;
    }

    auto models = m_engine->GetService<ModelLoader>();
    if (!models)
    {
        TraceLog(LOG_ERROR, "[UIManager] AutoLoadModelIfNeeded() - ModelLoader not available");
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
                         "[UIManager] AutoLoadModelIfNeeded() - Attempting to auto-load model "
                         "'%s' from %s",
                         requested.c_str(), resourcePath.c_str());
                if (models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                {
                    candidateName = stem.empty() ? requested : stem;
                    exists = true;
                    TraceLog(LOG_INFO,
                             "[UIManager] AutoLoadModelIfNeeded() - Auto-loaded model '%s'",
                             candidateName.c_str());
                    break;
                }
            }
        }
    }

    return exists;
}

void UIManager::CreateModelInstancesForMap()
{
    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] CreateModelInstancesForMap() - Engine not available");
        return;
    }

    auto levelManager = m_engine->GetLevelManager();
    auto models = m_engine->GetService<ModelLoader>();

    if (!levelManager || !models)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] CreateModelInstancesForMap() - Required services not available");
        return;
    }

    TraceLog(LOG_INFO,
             "[UIManager] CreateModelInstancesForMap() - Creating model instances for "
             "array-format map (%d objects)",
             levelManager->GetGameMap().GetMapObjects().size());
    for (const auto &object : levelManager->GetGameMap().GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string requested = object.modelName;
            std::string candidateName;

            if (!AutoLoadModelIfNeeded(requested, candidateName))
            {
                TraceLog(LOG_WARNING,
                         "[UIManager] CreateModelInstancesForMap() - Model '%s' not available "
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
                    "[UIManager] CreateModelInstancesForMap() - Failed to add instance for '%s'",
                    candidateName.c_str());
            }
            else
            {
                TraceLog(LOG_INFO,
                         "[UIManager] CreateModelInstancesForMap() - Added instance for '%s'",
                         candidateName.c_str());
            }
        }
        else if (object.type == MapObjectType::LIGHT)
        {
            TraceLog(LOG_INFO,
                     "[UIManager] CreateModelInstancesForMap() - Skipping LIGHT object '%s' for "
                     "model instance creation",
                     object.name.c_str());
        }
    }
}

void UIManager::LoadMapObjects(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "[UIManager] LoadMapObjects() - Loading map objects...");

    if (!m_engine)
    {
        TraceLog(LOG_ERROR, "[UIManager] LoadMapObjects() - Engine not available");
        throw std::runtime_error("Engine not available");
    }

    auto levelManager = m_engine->GetLevelManager();
    if (!levelManager)
    {
        TraceLog(LOG_ERROR, "[UIManager] LoadMapObjects() - LevelManager not available");
        throw std::runtime_error("LevelManager not available");
    }

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
                TraceLog(LOG_INFO,
                         "[UIManager] LoadMapObjects() - Detected array format, using LoadGameMap");

                MapLoader loader;
                levelManager->GetGameMap() = loader.LoadMap(mapPath.c_str());

                // Register any models that MapLoader preloaded into the GameMap
                RegisterPreloadedModels();

                // Create model instances for the loaded map objects
                CreateModelInstancesForMap();
            }
            else
            {
                // Assume standard JSON object format
                TraceLog(LOG_INFO, "[UIManager] LoadMapObjects() - Detected object format, using "
                                   "LoadMapObjects");
                levelManager->LoadEditorMap(mapPath);
            }
        }
        else
        {
            TraceLog(LOG_WARNING,
                     "[UIManager] LoadMapObjects() - Could not open file to detect format, "
                     "defaulting to LoadMapObjects: %s",
                     mapPath.c_str());
            levelManager->LoadEditorMap(mapPath);
        }

        TraceLog(LOG_INFO, "[UIManager] LoadMapObjects() - Map objects loaded successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[UIManager] LoadMapObjects() - Failed to load map objects: %s",
                 e.what());
        throw;
    }
}

void UIManager::HandleStartGameWithMap(bool *showMenu, bool *isGameInitialized)
{
    std::string selectedMapName = m_menu->GetSelectedMapName();
    TraceLog(LOG_INFO, "[UIManager] HandleStartGameWithMap() - Starting game with map: %s",
             selectedMapName.c_str());

    if (selectedMapName.empty())
    {
        TraceLog(LOG_WARNING, "[UIManager] HandleStartGameWithMap() - No map selected");
        return;
    }

    std::string mapPath = ConvertMapNameToPath(selectedMapName);
    TraceLog(LOG_INFO, "[UIManager] HandleStartGameWithMap() - Map path: %s", mapPath.c_str());

    if (!m_engine)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] HandleStartGameWithMap() - Required services not available");
        return;
    }

    auto levelManager = m_engine->GetLevelManager();
    PlayerController *playerController = nullptr;
    if (m_engine->GetModuleManager())
    {
        auto *module = m_engine->GetModuleManager()->GetModule("Player");
        if (module)
        {
            playerController = dynamic_cast<PlayerController *>(module);
        }
    }

    if (!levelManager || !playerController)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] HandleStartGameWithMap() - Required services not available");
        return;
    }

    m_menu->SetGameInProgress(true);
    // Map path is set by LoadEditorMap

    try
    {
        // 1. Analyze map for required models
        std::vector<std::string> requiredModels = AnalyzeMapForRequiredModels(mapPath);

        // 2. Load required models
        if (!LoadRequiredModels(requiredModels))
        {
            return;
        }

        // 3. Initialize collision system with models
        if (!InitializeCollisionSystemWithModels(requiredModels))
        {
            return;
        }

        // 4. Load map objects (now that models are loaded and collision is ready)
        LoadMapObjects(mapPath);

        // 5. Initialize player
        playerController->InitializePlayer();
        TraceLog(LOG_INFO,
                 "[UIManager] HandleStartGameWithMap() - Player initialized successfully");

        *showMenu = false;
        *isGameInitialized = true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "[UIManager] HandleStartGameWithMap() - Failed to start game with map: %s",
                 e.what());
        TraceLog(LOG_WARNING,
                 "[UIManager] HandleStartGameWithMap() - Game may not function correctly");
    }
}

void UIManager::HandleExitGame(bool *showMenu)
{
    TraceLog(LOG_INFO, "[UIManager] HandleExitGame() - Exiting game...");
    // CloseWindow(); // Let EngineApplication handle shutdown via ShouldClose()
    // But we can signal it if needed. For now, Raylib's CloseWindow or exit(0)
    // is often used in simple apps, but Engine should handle it.
    // EngineApplication checks WindowShouldClose().
    // We can't force it easily without an Exit flag in Engine.
    // Assuming exit(0) is acceptable for now or we should add RequestExit to Engine.
    std::exit(0);
}
