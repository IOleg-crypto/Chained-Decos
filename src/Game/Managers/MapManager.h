#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <raylib.h>

class GameMap;
class Player;
class CollisionManager;
class ModelLoader;
class RenderManager;
class Kernel;
class Menu;
class MapCollisionInitializer;

// Manages map loading, rendering and collision initialization
// Single Responsibility: Only handles map lifecycle
class MapManager
{
private:
    std::unique_ptr<GameMap> m_gameMap;
    std::string m_currentMapPath;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    RenderManager* m_renderManager;
    Kernel* m_kernel;
    Menu* m_menu;
    
    // Player spawn zone
    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone;
    bool m_spawnTextureLoaded;
    
    // Collision initializer
    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

public:
    MapManager(Player* player, CollisionManager* collisionManager, ModelLoader* models, RenderManager* renderManager, Kernel* kernel, Menu* menu = nullptr);
    ~MapManager();

    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void RenderSpawnZone() const;
    void DumpMapDiagnostics() const;
    
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);
    
    MapCollisionInitializer* GetCollisionInitializer() { return m_collisionInitializer.get(); }
    
    GameMap& GetGameMap();
    const std::string& GetCurrentMapPath() const { return m_currentMapPath; }
    
    Vector3 GetPlayerSpawnPosition() const;
    bool HasSpawnZone() const { return m_hasSpawnZone; }
    
    void SetPlayer(Player* player);
};

#endif // MAP_MANAGER_H

