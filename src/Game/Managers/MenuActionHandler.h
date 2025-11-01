#ifndef MENU_ACTION_HANDLER_H
#define MENU_ACTION_HANDLER_H

#include "Game/Menu/Menu.h"
#include "Game.h"
#include "Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "MapManager.h"
#include "ResourceManager.h"
#include "PlayerManager.h"
#include "Engine/Engine.h"

class MenuActionHandler
{
private:
    Game* m_game;
    Player* m_player;
    Menu* m_menu;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    MapManager* m_mapManager;
    ResourceManager* m_resourceManager;
    PlayerManager* m_playerManager;
    Engine* m_engine;
    bool* m_showMenu;
    bool* m_isGameInitialized;

    // Helper methods
    void HideMenuAndStartGame();
    void EnsurePlayerSafePosition();
    void ReinitializeCollisionSystemForResume();
    std::string ConvertMapNameToPath(const std::string& selectedMapName);
    std::vector<std::string> AnalyzeMapForRequiredModels(const std::string& mapPath);
    bool LoadRequiredModels(const std::vector<std::string>& requiredModels);
    bool InitializeCollisionSystemWithModels(const std::vector<std::string>& requiredModels);
    void LoadMapObjects(const std::string& mapPath);
    void RegisterPreloadedModels();
    void CreateModelInstancesForMap();
    bool AutoLoadModelIfNeeded(const std::string& requested, std::string& candidateName);

public:
    MenuActionHandler(Game* game, Player* player, Menu* menu, CollisionManager* collisionManager,
                      ModelLoader* models, MapManager* mapManager, ResourceManager* resourceManager,
                      PlayerManager* playerManager, Engine* engine,
                      bool* showMenu, bool* isGameInitialized);
    ~MenuActionHandler() = default;

    void HandleMenuActions();

    // Individual action handlers
    void HandleSinglePlayer();
    void HandleResumeGame();
    void HandleStartGameWithMap();
    void HandleExitGame();
};

#endif // MENU_ACTION_HANDLER_H

