#ifndef PLAYER_MODULE_H
#define PLAYER_MODULE_H

#include "Engine/Module/IEngineModule.h"
#include <memory>

// Forward declarations
class Kernel;
class Player;
class PlayerManager;
class CollisionManager;
class MapManager;
class ModelLoader;
class Engine;

// Модуль для управління гравцем та його логікою
class PlayerModule : public IEngineModule {
public:
    PlayerModule();
    ~PlayerModule() override = default;

    // IEngineModule interface
    const char* GetModuleName() const override { return "Player"; }
    const char* GetModuleVersion() const override { return "1.0.0"; }
    const char* GetModuleDescription() const override { return "Player management and gameplay logic"; }
    
    bool Initialize(Kernel* kernel) override;
    void Shutdown() override;
    
    void Update(float deltaTime) override;
    void Render() override;
    
    void RegisterServices(Kernel* kernel) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    Player* GetPlayer() const { return m_player; }
    PlayerManager* GetPlayerManager() const { return m_playerManager; }

private:
    // Dependencies (from Kernel) - тільки посилання, не власність
    Player* m_player;
    PlayerManager* m_playerManager;
    CollisionManager* m_collisionManager;
    MapManager* m_mapManager;
};

#endif // PLAYER_MODULE_H

