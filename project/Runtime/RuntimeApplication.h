#ifndef RUNTIME_APPLICATION_H
#define RUNTIME_APPLICATION_H

#include "core/application/IApplication.h"
#include "core/config/GameConfig.h"
#include <memory>
#include <scene/ecs/ECSRegistry.h>
#include <scene/ecs/Entity.h>

// Runtime application - uses full engine + own modules

namespace CHD
{

class RuntimeApplication : public CHEngine::IApplication
{
public:
    RuntimeApplication(int argc, char *argv[]);
    ~RuntimeApplication();

    // Lifecycle methods
    void OnConfigure(EngineConfig &config) override;
    void OnRegister() override;
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    void OnEvent(CHEngine::Event &e) override;

private:
    // Managers are now accessed through Engine services

    // ECS Entities
    entt::entity m_playerEntity = entt::null;

    // Game state
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
};

} // namespace CHD

#endif // RUNTIME_APPLICATION_H
