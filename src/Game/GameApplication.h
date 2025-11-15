#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "Engine/Application/Core/EngineApplication.h"
#include "Engine/CommandLineHandler/Core/CommandLineHandler.h"
#include "Engine/Audio/Core/AudioManager.h"
#include "Game/Player/Core/Player.h"
#include "Engine/Collision/Core/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include "Engine/World/Core/World.h"
#include "Menu/Menu.h"
// =================================================
#include <memory>

// Game application - uses full engine + own modules
class GameApplication : public EngineApplication {
public:
    GameApplication(int argc, char* argv[]);
    ~GameApplication();

protected:
    // Command line processing
    void ProcessCommandLine(int argc, char* argv[]) override;
    
    // Settings before initialization
    void OnPreInitialize() override;
    
    // Initialize Game components
    void OnInitializeServices() override;
    
    // Register Game modules (REQUIRED)
    void OnRegisterProjectModules() override;
    
    // Register Game services
    void OnRegisterProjectServices() override;
    
    // After initialization
    void OnPostInitialize() override;
    
    // Custom update logic
    void OnPostUpdate(float deltaTime) override;
    
    // Custom rendering logic
    void OnPostRender() override;
    
    // Before shutdown
    void OnPreShutdown() override;

private:
    // Basic engine components (created before system initialization)
    std::unique_ptr<CollisionManager> m_collisionManager;
    std::unique_ptr<ModelLoader> m_models;
    std::unique_ptr<WorldManager> m_world;
    
    // Game components are now created by systems:
    // - Player and PlayerManager → PlayerSystem
    // - Menu → UIController
    // - MapManager → MapSystem
    
    // Game manager components (created after system initialization)
    std::unique_ptr<AudioManager> m_soundSystem;
    
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

