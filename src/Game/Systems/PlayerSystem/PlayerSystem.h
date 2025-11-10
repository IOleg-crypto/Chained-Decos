#ifndef PLAYER_SYSTEM_H
#define PLAYER_SYSTEM_H

#include "Engine/Module/Interfaces/IEngineModule.h"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
class Kernel;
class Player;
class PlayerManager;
class CollisionManager;
class MapManager;
class ModelLoader;
class Engine;

// System for managing player and gameplay logic
// Creates and owns its components independently
class PlayerSystem : public IEngineModule {
public:
    PlayerSystem();
    ~PlayerSystem() override;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Player"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { 
        return "Player management and gameplay logic"; 
    }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    Player* GetPlayer() const { return m_player.get(); }
    PlayerManager* GetPlayerManager() const { return m_playerManager.get(); }

private:
    // System OWNS its components
    std::unique_ptr<Player> m_player;
    std::unique_ptr<PlayerManager> m_playerManager;
    
    // Kernel reference (for accessing services)
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    CollisionManager* m_collisionManager;
    MapManager* m_mapManager;
    ModelLoader* m_models;
    Engine* m_engine;
};

#endif // PLAYER_SYSTEM_H

