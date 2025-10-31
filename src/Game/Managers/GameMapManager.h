#ifndef GAME_MAP_MANAGER_H
#define GAME_MAP_MANAGER_H

#include <string>
#include <vector>
#include <Engine/Map/MapLoader.h>

class Player;
class CollisionManager;
class ModelLoader;
class RenderManager;

class GameMapManager
{
private:
    GameMap m_gameMap;
    std::string m_currentMapPath;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    RenderManager* m_renderManager;

public:
    GameMapManager(Player* player, CollisionManager* collisionManager, ModelLoader* models, RenderManager* renderManager);
    ~GameMapManager() = default;

    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void DumpMapDiagnostics() const;
    
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);
    
    GameMap &GetGameMap() { return m_gameMap; }
    const std::string& GetCurrentMapPath() const { return m_currentMapPath; }
};

#endif // GAME_MAP_MANAGER_H

