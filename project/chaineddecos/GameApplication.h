#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "core/application/IApplication.h"
#include "core/config/GameConfig.h"
#include "project/ChainedDecos/gamegui/Menu.h"
#include <memory>
#include <scene/ecs/ECSRegistry.h>
#include <scene/ecs/Entity.h>

// Game application - uses full engine + own modules

namespace CHD
{

class GameApplication : public CHEngine::IApplication
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
    void UpdatePlayerLogic();

    // In-game menu
    std::shared_ptr<Menu> m_menu;
};

} // namespace CHD

#endif // GAME_APPLICATION_H
