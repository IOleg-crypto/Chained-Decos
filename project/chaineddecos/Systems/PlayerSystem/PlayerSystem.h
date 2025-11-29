#ifndef PLAYER_SYSTEM_H
#define PLAYER_SYSTEM_H

#include "core/engine/EngineApplication.h"
#include "core/object/module/Interfaces/IEngineModule.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/audio/Core/AudioManager.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include <memory>
#include <string>
#include <vector>

// Forward declaration to avoid circular dependency
class MapSystem;

// System for managing player and gameplay logic
// Creates and owns its components independently
class PlayerSystem : public IEngineModule
{
public:
    PlayerSystem();
    ~PlayerSystem() override;

    // IEngineModule interface
    const char *GetModuleName() const override
    {
        return "Player";
    }
    const char *GetModuleVersion() const override
    {
        return "1.0.0";
    }
    const char *GetModuleDescription() const override
    {
        return "Player management and gameplay logic";
    }

    bool Initialize(Engine *engine) override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RegisterServices(Engine *engine) override;
    std::vector<std::string> GetDependencies() const override;

    // Accessors
    Player *GetPlayer() const
    {
        return m_player.get();
    }

    // Player management methods (from PlayerManager)
    void InitializePlayer();
    void UpdatePlayerLogic();

    // State management methods (from StateManager)
    void SavePlayerState(const std::string &currentMapPath);
    void RestorePlayerState();
    bool HasSavedState() const
    {
        return !m_savedMapPath.empty();
    }
    const std::string &GetSavedMapPath() const
    {
        return m_savedMapPath;
    }

private:
    static constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 1.5f;
    // System OWNS its components
    std::unique_ptr<Player> m_player;

    AudioManager *m_audioManager;

    // Dependencies obtained through Engine (references only)
    CollisionManager *m_collisionManager;
    MapSystem *m_mapSystem;
    ModelLoader *m_models;
    Engine *m_engine;

    // Saved state (from StateManager)
    std::string m_savedMapPath;
    Vector3 m_savedPlayerPosition;
    Vector3 m_savedPlayerVelocity;
};

#endif // PLAYER_SYSTEM_H
