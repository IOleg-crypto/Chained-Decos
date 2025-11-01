#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include <string>
#include <vector>
#include <Engine/Map/MapLoader.h>
#include "Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Menu/Menu.h"

class MapManager
{
private:
    GameMap m_gameMap;
    std::string m_currentMapPath;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    RenderManager* m_renderManager;
    Kernel* m_kernel;  // For collision service registration
    Menu* m_menu;      // For menu action checking (optional)

public:
    MapManager(Player* player, CollisionManager* collisionManager, ModelLoader* models, RenderManager* renderManager, Kernel* kernel, Menu* menu = nullptr);
    ~MapManager() = default;

    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void DumpMapDiagnostics() const;
    
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);
    
    GameMap &GetGameMap() { return m_gameMap; }
    const std::string& GetCurrentMapPath() const { return m_currentMapPath; }
};

#endif // MAP_MANAGER_H

