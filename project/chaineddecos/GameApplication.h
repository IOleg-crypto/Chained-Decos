#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "core/config/Core/GameConfig.h"
#include "core/engine/IApplication.h"
#include <core/ecs/ECSRegistry.h>
#include <core/ecs/Entity.h>

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
    // Managers are now accessed through Engine services

    // ECS Entities
    // ECS Entities
    entt::entity m_playerEntity = entt::null;
    Model m_playerModel = {0}; // Player model (generated at runtime)

    // Game state
    bool m_showMenu;
    bool m_isGameInitialized;
    bool m_showDebugCollision = false;
    bool m_showDebugStats = false;

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

    // Shader support
    Shader m_playerShader = {0};
    int m_locFallSpeed = -1;
    int m_locTime = -1;
    int m_locWindDir = -1;
    bool m_shaderLoaded = false;

    // HUD Font
    Font m_hudFont = {0};
    bool m_fontLoaded = false;
};

#endif // GAME_APPLICATION_H
