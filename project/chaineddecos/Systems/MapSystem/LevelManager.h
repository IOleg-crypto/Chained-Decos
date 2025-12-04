#ifndef MAP_SYSTEM_H
#define MAP_SYSTEM_H

#include "../../../core/engine/EngineApplication.h"

#include "../../../core/object/module/Interfaces/IEngineModule.h"
#include "../../../scene/main/Core/World.h"
#include "../../../scene/resources/map/Core/MapLoader.h"
#include "../../../scene/resources/model/Core/Model.h"
#include "../../../components/physics/collision/Core/CollisionManager.h"
#include "../../../components/rendering/Core/RenderManager.h"
#include "core/interfaces/ILevelManager.h"
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>


// Forward declarations to avoid circular dependencies
class Player;
class Menu;
class MapCollisionInitializer;

// Configuration for MapSystem
struct MapSystemConfig
{
    std::string resourcePath = "resources/maps";
    bool enableDebugRendering = false;
    bool enableSpawnZoneRendering = true;
};

// System for managing maps and levels
// Integrates all map loading, rendering, and collision initialization logic
class LevelManager : public ILevelManager, public IEngineModule
{
public: // Lifecycle
    explicit LevelManager(const MapSystemConfig &config = {});
    ~LevelManager() override;

public: // ILevelManager Implementation
    bool LoadMap(const std::string &path) override
    {
        LoadEditorMap(path);
        return true;
    }
    void UnloadMap() override
    { /* TODO: Implement unload */
    }
    bool IsMapLoaded() const override
    {
        return HasSpawnZone();
    }
    std::string GetCurrentMapName() const override
    {
        return GetCurrentMapPath();
    }
    Vector3 GetSpawnPosition() const override
    {
        return GetPlayerSpawnPosition();
    }

public: // IEngineModule Implementation
    const char *GetModuleName() const override
    {
        return "Map";
    }
    const char *GetModuleVersion() const override
    {
        return "1.0.0";
    }
    const char *GetModuleDescription() const override
    {
        return "Map and level management";
    }

    bool Initialize(Engine *engine) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Engine *engine) override;
    std::vector<std::string> GetDependencies() const override;

public: // Map Management
    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void RenderSpawnZone() const;
    void DumpMapDiagnostics() const;

public: // Collision Initialization
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);

public: // Accessors & Setters
    GameMap &GetGameMap();
    const std::string &GetCurrentMapPath() const
    {
        return m_currentMapPath;
    }
    Vector3 GetPlayerSpawnPosition() const;
    bool HasSpawnZone() const
    {
        return m_hasSpawnZone;
    }
    MapCollisionInitializer *GetCollisionInitializer()
    {
        return m_collisionInitializer.get();
    }

    void SetPlayer(Player *player);

private: // Configuration & State
    MapSystemConfig m_config;
    std::unique_ptr<GameMap> m_gameMap;
    std::string m_currentMapPath;

private: // Spawn Zone
    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone;
    bool m_spawnTextureLoaded;

private: // Sub-systems
    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

private: // Dependencies
    WorldManager *m_worldManager;
    CollisionManager *m_collisionManager;
    ModelLoader *m_modelLoader;
    RenderManager *m_renderManager;
    Player *m_player;
    Menu *m_menu;
    Engine *m_engine;
};

#endif // MAP_SYSTEM_H
