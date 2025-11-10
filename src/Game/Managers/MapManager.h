#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <Engine/Map/Core/MapLoader.h>
#include <Engine/Map/Core/MapService.h>
#include "../Player/Player.h"
#include "Engine/Collision/Manager/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include "Engine/Render/Manager/RenderManager.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "../Menu/Menu.h"
#include "MapCollisionInitializer.h"

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
    
    // Get collision initializer (for external use if needed)
    MapCollisionInitializer* GetCollisionInitializer() { return m_collisionInitializer.get(); }
    
    GameMap &GetGameMap() { return m_gameMap; }
    const std::string& GetCurrentMapPath() const { return m_currentMapPath; }
    
    // Get player spawn position from spawn zone (returns center of spawn zone)
    Vector3 GetPlayerSpawnPosition() const;
    bool HasSpawnZone() const { return m_hasSpawnZone; }
    
    // Update Player reference (used when PlayerSystem initializes after MapSystem)
    void SetPlayer(Player* player);
};

#endif // MAP_MANAGER_H

