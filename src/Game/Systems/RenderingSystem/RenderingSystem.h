#ifndef RENDERING_SYSTEM_H
#define RENDERING_SYSTEM_H

#include "Engine/Module/Interfaces/IEngineModule.h"
#include <memory>
#include <vector>
#include <string>

class Kernel;
class Player;
class MapManager;
class CollisionManager;
class ModelLoader;
class Engine;

class RenderingSystem : public IEngineModule {
public:
    RenderingSystem();
    ~RenderingSystem() override = default;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Rendering"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { 
        return "Game world and UI rendering"; 
    }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Rendering methods
    void RenderGameWorld();
    void RenderGameUI() const;

private:
    // Helper method for lazy loading dependencies
    void EnsureDependencies();

private:
    // Kernel reference
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    Player* m_player;
    MapManager* m_mapManager;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    Engine* m_engine;
    
    // Timer state for UI
    float m_gameTime;
};

#endif // RENDERING_SYSTEM_H

