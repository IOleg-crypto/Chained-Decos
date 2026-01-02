#ifndef CD_RUNTIME_RUNTIMEAPPLICATION_H
#define CD_RUNTIME_RUNTIMEAPPLICATION_H

#include "core/application/application.h"
#include "core/config/game_config.h"
#include "scene/core/entity.h"
#include "scene/core/scene.h"

// Runtime application - uses full engine + own layers

namespace CHD
{

class RuntimeApplication : public CHEngine::Application
{
public:
    RuntimeApplication(int argc, char *argv[]);
    ~RuntimeApplication();

    // event.handling
    void OnEvent(CHEngine::Event &e) override;

private:
    // Managers are now accessed through Engine services

    // Scene System (new architecture)
    std::shared_ptr<CHEngine::Scene> m_ActiveScene;

    CHEngine::Entity m_playerEntity;

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

#endif // CD_RUNTIME_RUNTIMEAPPLICATION_H
