#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "components/physics/collision/Core/CollisionManager.h"
#include "core/config/Core/GameConfig.h"
#include "core/engine/IApplication.h"
#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"
#include <core/ecs/ECSRegistry.h>
#include <core/ecs/Entity.h>
#include <memory>
#include <raylib.h>


// Game application - uses full engine + own modules
class GameApplication : public IApplication
{
public:
    GameApplication(int argc, char *argv[]);
    ~GameApplication();

    // IApplication interface - all methods receive Engine&
    void OnConfigure(EngineConfig &config) override;
    void OnRegister(Engine &engine) override;
    void OnStart(Engine &engine) override;
    void OnUpdate(float deltaTime, Engine &engine) override;
    void OnRender(Engine &engine) override;
    void OnShutdown() override;

private:
    // Basic engine components
    std::shared_ptr<CollisionManager> m_collisionManager;
    std::shared_ptr<ModelLoader> m_models;
    std::shared_ptr<WorldManager> m_world;

    // ECS Entities
    entt::entity m_playerEntity;
    Model m_playerModel;

    // Game state
    bool m_showMenu;
    bool m_isGameInitialized;
    bool m_showDebugCollision;
    bool m_showDebugStats;
    bool m_cursorDisabled;

    // Configuration
    GameConfig m_gameConfig;

    // Shader support
    Shader m_playerShader;
    int m_locFallSpeed;
    int m_locTime;
    int m_locWindDir;
    bool m_shaderLoaded;

    // HUD Font
    Font m_hudFont;
    bool m_fontLoaded;

    // Helper methods
    void InitInput(Engine &engine);
    void HandleMenuActions(Engine &engine);
    void UpdatePlayerLogic(Engine &engine);
    void SaveGameState();
};

#endif // GAME_APPLICATION_H
