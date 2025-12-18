#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "components/physics/collision/Core/CollisionManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/IPlayer.h"
#include "core/object/module/Interfaces/IEngineModule.h"
#include "scene/main/Core/MapCollisionInitializer.h"
#include "scene/main/Core/World.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include <memory>
#include <string>
#include <vector>

// Configuration for LevelManager
struct LevelManagerConfig
{
    std::string resourcePath = "resources/maps";
    bool enableDebugRendering = false;
    bool enableSpawnZoneRendering = true;
};

// Central System for managing maps and levels
class LevelManager : public ILevelManager
{
public:
    explicit LevelManager(const LevelManagerConfig &config = {});
    ~LevelManager() override;

    // ILevelManager Implementation
    bool LoadMap(const std::string &path) override;
    void UnloadMap() override;
    bool IsMapLoaded() const override;
    const std::string &GetCurrentMapPath() const override;
    std::string GetCurrentMapName() const override;
    Vector3 GetSpawnPosition() const override;

    // IEngineModule Implementation
    const char *GetModuleName() const override
    {
        return "LevelManager";
    }
    const char *GetModuleVersion() const override
    {
        return "1.1.0";
    }
    const char *GetModuleDescription() const override
    {
        return "Central Map and Level Management";
    }

    bool Initialize(Engine *engine) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Engine *engine) override;
    std::vector<std::string> GetDependencies() const override;

    // Map Management
    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap() override;
    void RenderSpawnZone() const;
    void DumpMapDiagnostics() const;

    // Collision Initialization
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels) override;

    // Accessors
    GameMap &GetGameMap();
    Vector3 GetPlayerSpawnPosition() const;
    bool HasSpawnZone() const
    {
        return m_hasSpawnZone;
    }
    MapCollisionInitializer *GetCollisionInitializer()
    {
        return m_collisionInitializer.get();
    }

    void SetPlayer(IPlayer *player);

private:
    LevelManagerConfig m_config;
    std::unique_ptr<GameMap> m_gameMap;
    std::string m_currentMapPath;

    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone = false;
    bool m_spawnTextureLoaded = false;

    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

    // Dependencies
    WorldManager *m_worldManager = nullptr;
    CollisionManager *m_collisionManager = nullptr;
    ModelLoader *m_modelLoader = nullptr;
    RenderManager *m_renderManager = nullptr;
    IPlayer *m_player = nullptr;
    IMenu *m_menu = nullptr;
    Engine *m_engine = nullptr;
};

#endif // LEVELMANAGER_H
