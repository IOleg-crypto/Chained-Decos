#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "IWorldManager.h"
#include "MapCollisionInitializer.h"
#include "World.h"
#include "components/physics/collision/core/collisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IEngineModule.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/model/Model.h"
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
    void SetActiveScene(std::shared_ptr<CHEngine::Scene> scene) override;
    bool LoadScene(const std::string &path) override;
    bool LoadSceneByIndex(int index) override;
    bool LoadSceneByName(const std::string &name) override;
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

    bool Initialize(IEngine *engine) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(IEngine *engine) override;
    std::vector<std::string> GetDependencies() const override;

    // Map Management
    void LoadEditorMap(const std::string &mapPath);
    std::string ConvertMapNameToPath(const std::string &mapName);
    void RenderEditorMap() override;
    void RefreshMapEntities() override;
    void RefreshUIEntities() override;
    void SyncEntitiesToMap() override;
    void RenderSpawnZone() const;
    void DumpMapDiagnostics() const;

    // Collision Initialization
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels) override;

    // Accessors
    GameScene &GetGameScene() override;
    Vector3 GetPlayerSpawnPosition() const;
    bool HasSpawnZone() const
    {
        return m_hasSpawnZone;
    }
    MapCollisionInitializer *GetCollisionInitializer()
    {
        return m_collisionInitializer.get();
    }

    void SetPlayer(std::shared_ptr<IPlayer> player);

private:
    LevelManagerConfig m_config;
    std::shared_ptr<GameScene> m_gameScene;
    std::string m_currentMapPath;
    std::vector<std::string> m_scenes;

    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone = false;
    bool m_spawnTextureLoaded = false;

    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

    // Dependencies
    IWorldManager *m_worldManager;
    CollisionManager *m_collisionManager;
    ModelLoader *m_modelLoader;
    RenderManager *m_renderManager;
    std::shared_ptr<IPlayer> m_player;
    std::shared_ptr<IMenu> m_menu;
    IEngine *m_engine = nullptr;
    std::shared_ptr<CHEngine::Scene> m_activeScene;
};

#endif // LEVELMANAGER_H
