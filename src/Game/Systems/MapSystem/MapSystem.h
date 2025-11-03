#ifndef MAP_SYSTEM_H
#define MAP_SYSTEM_H

#include "Engine/Module/IEngineModule.h"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Kernel;
class MapManager;
class WorldManager;
class CollisionManager;
class ModelLoader;
class RenderManager;
class Player;
class Menu;
class Engine;

// System for managing maps and levels
// Creates and owns its components independently
class MapSystem : public IEngineModule {
public:
    MapSystem();
    ~MapSystem() override;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Map"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { 
        return "Map and level management"; 
    }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    MapManager* GetMapManager() const { return m_mapManager.get(); }

private:
    // System OWNS its components
    std::unique_ptr<MapManager> m_mapManager;
    
    // Kernel reference (for accessing services)
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    WorldManager* m_worldManager;
    CollisionManager* m_collisionManager;
    ModelLoader* m_modelLoader;
    RenderManager* m_renderManager;
    Player* m_player;
    Menu* m_menu;
    Engine* m_engine;
};

#endif // MAP_SYSTEM_H

