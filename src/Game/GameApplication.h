#ifndef GAME_APPLICATION_H
#define GAME_APPLICATION_H

#include "Engine/Application/EngineApplication.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"
#include <memory>

// Forward declarations
class Player;
class CollisionManager;
class ModelLoader;
class WorldManager;
class Menu;
class MapManager;
class ResourceManager;
class StateManager;
class GameRenderHelpers;
class PlayerManager;
class UpdateManager;
class MenuActionHandler;

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
    std::unique_ptr<ResourceManager> m_modelManager;
    std::unique_ptr<StateManager> m_stateManager;
    std::unique_ptr<GameRenderHelpers> m_renderHelper;
    std::unique_ptr<UpdateManager> m_updateManager;
    // GameRenderManager replaced with RenderingSystem
    std::unique_ptr<MenuActionHandler> m_menuActionHandler;
    
    // Game state
    bool m_showMenu;
    bool m_isGameInitialized;
    
    // Cursor state tracking to avoid calling DisableCursor/EnableCursor every frame
    bool m_cursorDisabled;
    
    // Command line configuration
    GameConfig m_gameConfig;
    
    // Helper methods
    void RegisterCoreKernelServices();
    void RegisterManagerKernelServices();
    void InitializeManagers();
    void InitInput();
    void HandleMenuActions();
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    
    // Game state management
    void SaveGameState();
};

#endif // GAME_APPLICATION_H

