#ifndef MAP_SYSTEM_H
#define MAP_SYSTEM_H

#include "servers/physics/collision/Core/CollisionManager.h"
#include "platform/windows/Core/EngineApplication.h"
#include "core/object/kernel/Core/Kernel.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include "core/object/module/Interfaces/IEngineModule.h"
#include "servers/rendering/Core/RenderManager.h"
#include "scene/main/Core/World.h"
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
class MapSystem : public IEngineModule
{
public:
    explicit MapSystem(const MapSystemConfig &config = {});
    ~MapSystem() override;

    // IEngineModule interface
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

    bool Initialize(Kernel *kernel) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Kernel *kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Map loading and management
    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void RenderSpawnZone() const;
    void DumpMapDiagnostics() const;

    // Collision initialization
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);

    // Accessors
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

private:
    // Configuration
    MapSystemConfig m_config;

    // Map data
    std::unique_ptr<GameMap> m_gameMap;
    std::string m_currentMapPath;

    // Player spawn zone
    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone;
    bool m_spawnTextureLoaded;

    // Collision initializer
    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

    // Kernel reference (for accessing services)
    Kernel *m_kernel;

    // Dependencies obtained through Kernel (references only)
    WorldManager *m_worldManager;
    CollisionManager *m_collisionManager;
    ModelLoader *m_modelLoader;
    RenderManager *m_renderManager;
    Player *m_player;
    Menu *m_menu;
    Engine *m_engine;
};

#include "core/object/kernel/Interfaces/IKernelService.h"

struct MapSystemService : public IKernelService
{
    MapSystem *mapSystem = nullptr;
    explicit MapSystemService(MapSystem *ms) : mapSystem(ms)
    {
    }
    bool Initialize() override
    {
        return mapSystem != nullptr;
    }
    void Shutdown() override
    {
    }
    void Update(float deltaTime) override
    {
    }
    void Render() override
    {
    }
    const char *GetName() const override
    {
        return "MapSystemService";
    }
};

#endif // MAP_SYSTEM_H
