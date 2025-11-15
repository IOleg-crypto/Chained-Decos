#ifndef PLAYER_SYSTEM_H
#define PLAYER_SYSTEM_H

#include "Engine/Module/Interfaces/IEngineModule.h"
#include "Engine/Audio/Core/AudioManager.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "Game/Player/Core/Player.h"
#include "Engine/Collision/Core/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include "Engine/Engine.h"
#include <memory>
#include <vector>
#include <string>

class MapSystem;

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

    // Player management methods (from PlayerManager)
    void InitializePlayer();
    void UpdatePlayerLogic();

    // State management methods (from StateManager)
    void SavePlayerState(const std::string& currentMapPath);
    void RestorePlayerState();
    bool HasSavedState() const { return !m_savedMapPath.empty(); }
    const std::string& GetSavedMapPath() const { return m_savedMapPath; }

private:
    static constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 1.5f;
    // System OWNS its components
    std::unique_ptr<Player> m_player;

    AudioManager* m_audioManager;
    // Kernel reference (for accessing services)
    Kernel* m_kernel;
    
    // Dependencies obtained through Kernel (references only)
    CollisionManager* m_collisionManager;
    MapSystem* m_mapSystem;
    ModelLoader* m_models;
    Engine* m_engine;

    // Saved state (from StateManager)
    std::string m_savedMapPath;
    Vector3 m_savedPlayerPosition;
    Vector3 m_savedPlayerVelocity;
};

#endif // PLAYER_SYSTEM_H

