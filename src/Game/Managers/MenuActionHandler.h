#ifndef MENU_ACTION_HANDLER_H
#define MENU_ACTION_HANDLER_H

#include "Game/Menu/Menu.h"
#include "Engine/Kernel/Core/Kernel.h"

// Forward declarations to break circular dependencies
class Player;
class CollisionManager;
class ModelLoader;
class MapManager;
class ResourceManager;
class PlayerManager;
class StateManager;
class Engine;

class MenuActionHandler
{
private:
    Kernel* m_kernel;
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

    // Helper methods to get services from kernel
    Player* GetPlayer() const;
    Menu* GetMenu() const;
    CollisionManager* GetCollisionManager() const;
    ModelLoader* GetModels() const;
    MapManager* GetMapManager() const;
    ResourceManager* GetResourceManager() const;
    PlayerManager* GetPlayerManager() const;
    Engine* GetEngine() const;
    class StateManager* GetStateManager() const;

public:
    MenuActionHandler(Kernel* kernel, bool* showMenu, bool* isGameInitialized);
    ~MenuActionHandler() = default;

    void HandleMenuActions();

    // Individual action handlers
    void HandleSinglePlayer();
    void HandleResumeGame();
    void HandleStartGameWithMap();
    void HandleExitGame();
};

#endif // MENU_ACTION_HANDLER_H

