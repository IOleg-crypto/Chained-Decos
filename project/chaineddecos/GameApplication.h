#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "core/config/Core/GameConfig.h"
#include "core/engine/IApplication.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/audio/Core/AudioManager.h"
#include "servers/physics/collision/Core/CollisionManager.h"
// =================================================
#include <memory>

// Game application - uses full engine + own modules
class GameApplication : public IApplication
{
public:
    GameApplication(int argc, char *argv[]);
    ~GameApplication();

    // Lifecycle methods
    void OnConfigure(EngineConfig &config) override;
    void OnRegister() override;
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;

private:
    // Basic engine components (created before system initialization)
    std::shared_ptr<CollisionManager> m_collisionManager;
    std::shared_ptr<ModelLoader> m_models;
    std::shared_ptr<WorldManager> m_world;

    // Game components are now created by systems:
    // - Player and PlayerManager → PlayerSystem
    // - Menu → UIController
    // - MapManager → MapSystem

    // Game manager components (created after system initialization)
    std::shared_ptr<AudioManager> m_soundSystem;

    // Game state
    bool m_showMenu;
    bool m_isGameInitialized;

    // Cursor state tracking to avoid calling DisableCursor/EnableCursor every frame
    bool m_cursorDisabled;

    // Command line configuration
    GameConfig m_gameConfig;

    // Helper methods
    void InitInput();
    void HandleMenuActions();
    void UpdatePlayerLogic();

    // Game state management
    void SaveGameState();
};

#endif // GAME_APPLICATION_H
