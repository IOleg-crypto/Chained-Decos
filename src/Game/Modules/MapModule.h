#ifndef MAP_MODULE_H
#define MAP_MODULE_H

#include "Engine/Module/IEngineModule.h"
#include <memory>

// Forward declarations
class Kernel;
class MapManager;
class WorldManager;
class CollisionManager;
class ModelLoader;
class Player;
class RenderManager;
class Menu;

// Модуль для управління картами та рівнями
class MapModule : public IEngineModule {
public:
    MapModule();
    ~MapModule() override = default;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Map"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { return "Map and level management"; }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    
    void Update(float deltaTime) override;
    void Render() override;
    
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    MapManager* GetMapManager() const { return m_mapManager; }

private:
    // Dependencies (from Kernel) - тільки посилання, не власність
    MapManager* m_mapManager;
    WorldManager* m_worldManager;
    CollisionManager* m_collisionManager;
    ModelLoader* m_modelLoader;
};

#endif // MAP_MODULE_H

