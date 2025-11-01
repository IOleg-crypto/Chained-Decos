#include "MenuActionHandler.h"
#include "../Game.h"
#include "../Player/Player.h"
#include "Game/Menu/Menu.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Map/MapLoader.h"
#include "MapManager.h"
#include "ResourceManager.h"
#include "PlayerManager.h"
#include "Engine/Engine.h"
#include <raylib.h>
#include <filesystem>
#include <fstream>
#include <algorithm>

MenuActionHandler::MenuActionHandler(Game* game, Player* player, Menu* menu,
                                      CollisionManager* collisionManager, ModelLoader* models,
                                      MapManager* mapManager, ResourceManager* resourceManager,
                                      PlayerManager* playerManager, Engine* engine,
                                      bool* showMenu, bool* isGameInitialized)
    : m_game(game), m_player(player), m_menu(menu), m_collisionManager(collisionManager),
      m_models(models), m_mapManager(mapManager), m_resourceManager(resourceManager),
      m_playerManager(playerManager), m_engine(engine),
      m_showMenu(showMenu), m_isGameInitialized(isGameInitialized)
{
    TraceLog(LOG_INFO, "MenuActionHandler created");
}

void MenuActionHandler::HandleMenuActions()
{
    MenuAction action = m_menu->ConsumeAction();
    switch (action)
    {
    case MenuAction::SinglePlayer:
        HandleSinglePlayer();
        break;
    case MenuAction::ResumeGame:
        HandleResumeGame();
        break;
    case MenuAction::StartGameWithMap:
        HandleStartGameWithMap();
        break;
    case MenuAction::ExitGame:
        HandleExitGame();
        break;
    default:
        break;
    }
}

void MenuActionHandler::HandleSinglePlayer()
{
    TraceLog(LOG_INFO, "MenuActionHandler::HandleSinglePlayer() - Starting singleplayer...");
    m_menu->SetGameInProgress(true);

    // Initialize player after map is loaded
    try
    {
        m_playerManager->InitPlayer();
        TraceLog(LOG_INFO, "MenuActionHandler::HandleSinglePlayer() - Player initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::HandleSinglePlayer() - Failed to initialize player: %s",
                 e.what());
        TraceLog(LOG_WARNING, "MenuActionHandler::HandleSinglePlayer() - Player may not render correctly");
    }

    m_game->ToggleMenu();
    *m_isGameInitialized = true; // Mark game as initialized
}

void MenuActionHandler::HideMenuAndStartGame()
{
    *m_showMenu = false;
    if (m_game)
    {
        m_game->HideCursor();
    }
    m_menu->ResetAction();
}

void MenuActionHandler::EnsurePlayerSafePosition()
{
    if (m_player->GetPlayerPosition().x == 0.0f &&
        m_player->GetPlayerPosition().y == 0.0f && m_player->GetPlayerPosition().z == 0.0f)
    {
        TraceLog(LOG_INFO, "MenuActionHandler::EnsurePlayerSafePosition() - Player position is origin, "
                           "resetting to safe position");
        m_player->SetPlayerPosition({0.0f, 2.0f, 0.0f}); // PLAYER_SAFE_SPAWN_HEIGHT = 2.0f
    }

    // Re-setup player collision and movement
    m_player->GetMovement()->SetCollisionManager(m_collisionManager);
    m_player->UpdatePlayerBox();
    m_player->UpdatePlayerCollision();
}

void MenuActionHandler::ReinitializeCollisionSystemForResume()
{
    TraceLog(LOG_WARNING, "MenuActionHandler::ReinitializeCollisionSystemForResume() - No colliders found, reinitializing...");
    std::vector<std::string> requiredModels = m_resourceManager->GetModelsRequiredForMap(m_mapManager->GetCurrentMapPath());

    // Reinitialize collision system safely
    try
    {
        // Clear existing colliders
        m_collisionManager->ClearColliders();

        // Initialize collision manager
        m_collisionManager->Initialize();

        // Try to create model collisions, but don't fail if it doesn't work
        try
        {
            m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
            TraceLog(LOG_INFO, "MenuActionHandler::ReinitializeCollisionSystemForResume() - Resume model collisions "
                               "created successfully");
        }
        catch (const std::exception &modelCollisionException)
        {
            TraceLog(LOG_WARNING,
                     "MenuActionHandler::ReinitializeCollisionSystemForResume() - Resume model collision creation "
                     "failed: %s",
                     modelCollisionException.what());
            TraceLog(LOG_WARNING, "MenuActionHandler::ReinitializeCollisionSystemForResume() - Continuing with basic "
                                  "collision system only");
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR,
                 "MenuActionHandler::ReinitializeCollisionSystemForResume() - Failed to reinitialize collision system "
                 "for resume: %s",
                 e.what());
    }
}

void MenuActionHandler::HandleResumeGame()
{
    TraceLog(LOG_INFO, "MenuActionHandler::HandleResumeGame() - Resuming game...");
    m_menu->SetAction(MenuAction::SinglePlayer);

    // Ensure game is properly initialized for resume
    if (!(*m_isGameInitialized))
    {
        TraceLog(LOG_INFO, "MenuActionHandler::HandleResumeGame() - Initializing game for resume...");

        // Load models for the current map (use saved map)
        std::vector<std::string> requiredModels = m_resourceManager->GetModelsRequiredForMap(m_mapManager->GetCurrentMapPath());
        m_resourceManager->LoadGameModelsSelective(requiredModels);

        // Initialize basic collision system first
        if (!m_game->InitCollisionsWithModelsSafe(requiredModels))
        {
            TraceLog(LOG_ERROR, "MenuActionHandler::HandleResumeGame() - Failed to initialize basic "
                                "collision system for singleplayer");
            TraceLog(LOG_ERROR,
                     "MenuActionHandler::HandleResumeGame() - Cannot continue without collision system");
            return;
        }
        TraceLog(LOG_INFO,
                 "MenuActionHandler::HandleResumeGame() - Collision system initialized for singleplayer");

        // Initialize player after map is loaded
        try
        {
            m_playerManager->InitPlayer();
            TraceLog(LOG_INFO, "MenuActionHandler::HandleResumeGame() - Player initialized for resume");
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR,
                     "MenuActionHandler::HandleResumeGame() - Failed to initialize player for resume: %s",
                     e.what());
            TraceLog(LOG_WARNING,
                     "MenuActionHandler::HandleResumeGame() - Player may not render correctly");
        }
    }
    else
    {
        // Game is already initialized, just ensure collision system is ready
        if (m_collisionManager->GetColliders().empty())
        {
            ReinitializeCollisionSystemForResume();
        }

        // Ensure player is properly positioned and set up
        EnsurePlayerSafePosition();
    }

    // Hide the menu and resume the game
    HideMenuAndStartGame();
    TraceLog(LOG_INFO, "MenuActionHandler::HandleResumeGame() - Game resumed successfully");
}

std::string MenuActionHandler::ConvertMapNameToPath(const std::string& selectedMapName)
{
    std::string mapPath;
    if (selectedMapName.length() >= 3 && isalpha(selectedMapName[0]) &&
        selectedMapName[1] == ':' &&
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

std::vector<std::string> MenuActionHandler::AnalyzeMapForRequiredModels(const std::string& mapPath)
{
    TraceLog(LOG_INFO, "MenuActionHandler::AnalyzeMapForRequiredModels() - Analyzing map to determine required models...");
    std::vector<std::string> requiredModels;

    try
    {
        requiredModels = m_resourceManager->GetModelsRequiredForMap(mapPath);
        if (requiredModels.empty())
        {
            TraceLog(LOG_WARNING, "MenuActionHandler::AnalyzeMapForRequiredModels() - No models required for map, but player model is always needed");
            requiredModels.emplace_back("player_low"); // Always include player model
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::AnalyzeMapForRequiredModels() - Failed to analyze map for required models: %s", e.what());
        TraceLog(LOG_ERROR, "MenuActionHandler::AnalyzeMapForRequiredModels() - Cannot continue without model analysis");
        throw;
    }

    TraceLog(LOG_INFO, "MenuActionHandler::AnalyzeMapForRequiredModels() - Required models for map:");
    for (const auto &model : requiredModels)
    {
        TraceLog(LOG_INFO, "MenuActionHandler::AnalyzeMapForRequiredModels() -   - %s", model.c_str());
    }
    TraceLog(LOG_INFO, "MenuActionHandler::AnalyzeMapForRequiredModels() - Total models required: %d", requiredModels.size());
    
    return requiredModels;
}

bool MenuActionHandler::LoadRequiredModels(const std::vector<std::string>& requiredModels)
{
    TraceLog(LOG_INFO, "MenuActionHandler::LoadRequiredModels() - Loading required models selectively...");
    auto loadResult = m_resourceManager->LoadGameModelsSelective(requiredModels);
    if (!loadResult || loadResult->loadedModels == 0)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::LoadRequiredModels() - Failed to load any required models");
        TraceLog(LOG_ERROR, "MenuActionHandler::LoadRequiredModels() - Cannot continue without models");
        return false;
    }
    TraceLog(LOG_INFO, "MenuActionHandler::LoadRequiredModels() - Successfully loaded %d/%d required models in %.2f seconds",
             loadResult->loadedModels, loadResult->totalModels, loadResult->loadingTime);
    return true;
}

bool MenuActionHandler::InitializeCollisionSystemWithModels(const std::vector<std::string>& requiredModels)
{
    TraceLog(LOG_INFO, "MenuActionHandler::InitializeCollisionSystemWithModels() - Initializing collision system with required models...");
    if (!m_game->InitCollisionsWithModelsSafe(requiredModels))
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::InitializeCollisionSystemWithModels() - Failed to initialize collision system with required models");
        TraceLog(LOG_ERROR, "MenuActionHandler::InitializeCollisionSystemWithModels() - Cannot continue without collision system");
        return false;
    }
    TraceLog(LOG_INFO, "MenuActionHandler::InitializeCollisionSystemWithModels() - Collision system initialized successfully");
    return true;
}

void MenuActionHandler::RegisterPreloadedModels()
{
    if (!m_mapManager->GetGameMap().loadedModels.empty())
    {
        TraceLog(LOG_INFO,
                 "MenuActionHandler::RegisterPreloadedModels() - Registering %d preloaded models from map into ModelLoader",
                 m_mapManager->GetGameMap().loadedModels.size());
        for (const auto &p : m_mapManager->GetGameMap().loadedModels)
        {
            const std::string &modelName = p.first;
            const ::Model &loaded = p.second;

            // Validate model before registration
            if (loaded.meshCount > 0)
            {
                if (m_models->RegisterLoadedModel(modelName, loaded))
                {
                    TraceLog(LOG_INFO,
                             "MenuActionHandler::RegisterPreloadedModels() - Successfully registered model from map: %s (meshCount: %d)",
                             modelName.c_str(), loaded.meshCount);
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "MenuActionHandler::RegisterPreloadedModels() - Failed to register model from map: %s",
                             modelName.c_str());
                }
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "MenuActionHandler::RegisterPreloadedModels() - Skipping invalid model from map: %s (meshCount: %d)",
                         modelName.c_str(), loaded.meshCount);
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "MenuActionHandler::RegisterPreloadedModels() - No preloaded models in GameMap to register");
    }
}

bool MenuActionHandler::AutoLoadModelIfNeeded(const std::string& requested, std::string& candidateName)
{
    auto available = m_models->GetAvailableModels();
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
                std::string resourcePath = std::string(PROJECT_ROOT_DIR) + "/resources/" + requested;
                if (std::filesystem::path(requested).extension().empty())
                    resourcePath = std::string(PROJECT_ROOT_DIR) + "/resources/" + requested + ext;

                TraceLog(LOG_INFO,
                         "MenuActionHandler::AutoLoadModelIfNeeded() - Attempting to auto-load model '%s' from %s",
                         requested.c_str(), resourcePath.c_str());
                if (m_models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                {
                    candidateName = stem.empty() ? requested : stem;
                    exists = true;
                    TraceLog(LOG_INFO, "MenuActionHandler::AutoLoadModelIfNeeded() - Auto-loaded model '%s'", candidateName.c_str());
                    break;
                }
            }
        }
    }

    return exists;
}

void MenuActionHandler::CreateModelInstancesForMap()
{
    TraceLog(LOG_INFO,
             "MenuActionHandler::CreateModelInstancesForMap() - Creating model instances for array-format map (%d objects)",
             m_mapManager->GetGameMap().objects.size());
    for (const auto &object : m_mapManager->GetGameMap().objects)
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string requested = object.modelName;
            std::string candidateName;

            if (!AutoLoadModelIfNeeded(requested, candidateName))
            {
                TraceLog(LOG_WARNING,
                         "MenuActionHandler::CreateModelInstancesForMap() - Model '%s' not available after auto-load attempts; skipping instance for object '%s'",
                         requested.c_str(), object.name.c_str());
                continue;
            }

            ModelInstanceConfig cfg;
            cfg.position = object.position;
            cfg.rotation = object.rotation;
            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                            ? object.scale.x : 1.0f;
            cfg.color = object.color;
            cfg.spawn = true;

            if (!m_models->AddInstanceEx(candidateName, cfg))
            {
                TraceLog(LOG_WARNING, "MenuActionHandler::CreateModelInstancesForMap() - Failed to add instance for '%s'", candidateName.c_str());
            }
            else
            {
                TraceLog(LOG_INFO, "MenuActionHandler::CreateModelInstancesForMap() - Added instance for '%s'", candidateName.c_str());
            }
        }
        else if (object.type == MapObjectType::LIGHT)
        {
            TraceLog(LOG_INFO, "MenuActionHandler::CreateModelInstancesForMap() - Skipping LIGHT object '%s' for model instance creation", object.name.c_str());
        }
    }
}

void MenuActionHandler::LoadMapObjects(const std::string& mapPath)
{
    TraceLog(LOG_INFO, "MenuActionHandler::LoadMapObjects() - Loading map objects...");
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
                TraceLog(LOG_INFO, "MenuActionHandler::LoadMapObjects() - Detected array format, using LoadGameMap");
                m_mapManager->GetGameMap() = LoadGameMap(mapPath.c_str());

                // Register any models that MapLoader preloaded into the GameMap
                RegisterPreloadedModels();

                // Create model instances
                CreateModelInstancesForMap();
            }
            else
            {
                TraceLog(LOG_INFO, "MenuActionHandler::LoadMapObjects() - Detected editor format, using LoadEditorMap");
                m_game->LoadEditorMap(mapPath);
            }
        }
        else
        {
            TraceLog(LOG_ERROR, "MenuActionHandler::LoadMapObjects() - Cannot open map file: %s", mapPath.c_str());
            throw std::runtime_error("Cannot open map file");
        }

        TraceLog(LOG_INFO, "MenuActionHandler::LoadMapObjects() - Map loaded successfully with %d objects", m_mapManager->GetGameMap().objects.size());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::LoadMapObjects() - Failed to load map: %s", e.what());
        TraceLog(LOG_ERROR, "MenuActionHandler::LoadMapObjects() - Cannot continue without map");
        throw;
    }
}

void MenuActionHandler::HandleStartGameWithMap()
{
    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Starting game with selected map...");
    m_menu->SetGameInProgress(true);
    std::string selectedMapName = m_menu->GetSelectedMapName();
    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Selected map: %s", selectedMapName.c_str());

    // Convert map name to full path
    std::string mapPath = ConvertMapNameToPath(selectedMapName);
    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Full map path: %s", mapPath.c_str());

    // Step 1: Analyze map to determine required models
    std::vector<std::string> requiredModels;
    try
    {
        requiredModels = AnalyzeMapForRequiredModels(mapPath);
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::HandleStartGameWithMap() - Failed to analyze map: %s", e.what());
        return;
    }

    // Step 2: Load only the required models selectively
    if (!LoadRequiredModels(requiredModels))
    {
        return;
    }

    // Step 3: Initialize collision system with required models
    if (!InitializeCollisionSystemWithModels(requiredModels))
    {
        return;
    }

    // Step 4: Load the map objects
    try
    {
        LoadMapObjects(mapPath);
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::HandleStartGameWithMap() - Failed to load map: %s", e.what());
        return;
    }

    // Step 5: Initialize player after map is loaded
    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Initializing player...");
    try
    {
        m_playerManager->InitPlayer();
        TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Player initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "MenuActionHandler::HandleStartGameWithMap() - Failed to initialize player: %s", e.what());
        TraceLog(LOG_WARNING, "MenuActionHandler::HandleStartGameWithMap() - Player may not render correctly");
    }

    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Game initialization complete");
    *m_isGameInitialized = true;

    // Hide menu and start the game
    TraceLog(LOG_INFO, "MenuActionHandler::HandleStartGameWithMap() - Hiding menu and starting game...");
    HideMenuAndStartGame();
}

void MenuActionHandler::HandleExitGame()
{
    TraceLog(LOG_INFO, "MenuActionHandler::HandleExitGame() - Exit game requested from menu.");
    // Clear game state when exiting
    m_menu->SetGameInProgress(false);
    *m_showMenu = true; // Show menu one last time before exit
    if (m_engine)
    {
        m_engine->RequestExit();
    }
    m_menu->ResetAction();
}

