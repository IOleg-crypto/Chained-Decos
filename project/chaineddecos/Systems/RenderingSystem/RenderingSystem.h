#ifndef RENDERING_SYSTEM_H
#define RENDERING_SYSTEM_H

#include "core/engine/EngineApplication.h"
#include "core/object/module/Interfaces/IEngineModule.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations to avoid circular dependencies
class Player;
class MapSystem;
class CollisionManager;
class ModelLoader;

class RenderingSystem : public IEngineModule
{
public:
    RenderingSystem();
    ~RenderingSystem() override = default;

    // IEngineModule interface
    const char *GetModuleName() const override
    {
        return "Rendering";
    }
    const char *GetModuleVersion() const override
    {
        return "1.0.0";
    }
    const char *GetModuleDescription() const override
    {
        return "Game world and UI rendering";
    }

    bool Initialize(Engine *engine) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Engine *engine) override;
    std::vector<std::string> GetDependencies() const override;

    // Rendering methods
    void RenderGameWorld();
    void RenderGameUI() const;

private:
    // Helper method for lazy loading dependencies
    void EnsureDependencies();

private:
    // Dependencies obtained through Engine (references only)
    Player *m_player;
    MapSystem *m_mapSystem;
    CollisionManager *m_collisionManager;
    ModelLoader *m_models;
    Engine *m_engine;

    // Timer state for UI
    float m_gameTime;
};

#endif // RENDERING_SYSTEM_H
