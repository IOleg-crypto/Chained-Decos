#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Menu/Menu.h"
#include "Player/Player.h"
#include "Map/MapLoader.h"  // For loading editor-created maps

class Game
{
private:
    static Game* s_instance;
    Player m_player;
    CollisionManager m_collisionManager;
    ModelLoader m_models;
    WorldManager m_world;
    Menu m_menu;
    Engine *m_engine = nullptr;

    // Map loading system
    GameMap m_gameMap;                   // New comprehensive map system

private:
    bool m_showMenu;
    bool m_isGameInitialized;
    [[maybe_unused]] bool m_isDebugInfo;

    // Game state saving members
    std::string m_savedMapPath;
    std::string m_currentMapPath;
    Vector3 m_savedPlayerPosition;
    Vector3 m_savedPlayerVelocity;

public:
    Game(Engine *engine);
    ~Game();

    void Init();
    void Run();

    void ToggleMenu();
    void RequestExit() const;
    bool IsRunning() const;

    // Cursor management
    void EnableCursor();
    void HideCursor();

    void Update();
    void Render();
    void InitInput();
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string>& requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string>& requiredModels);
    void InitPlayer();
    void LoadGameModels();
    void LoadGameModelsSelective(const std::vector<std::string>& modelNames);
    void LoadGameModelsSelectiveSafe(const std::vector<std::string>& modelNames);
    std::string GetModelNameForObjectType(int objectType, const std::string& modelName = "");
    std::vector<std::string> GetModelsRequiredForMap(const std::string& mapIdentifier);
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    void Cleanup(); // Resource cleanup
    //void HandleKeyboardShortcuts(); maybe implemented in the future
    void HandleMenuActions();
    void RenderGameWorld();
    void RenderGameUI() const;

    // Helper functions for code quality and reduced duplication
    void CreatePlatform(const Vector3& position, const Vector3& size, Color color, CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize) ;

    // Map loading and rendering
    void LoadEditorMap(const std::string& mapPath);
    void RenderEditorMap();

    // Game state management
    void SaveGameState();
    void RestoreGameState();

    // Singleton methods
    static Game* GetInstance();

    // Test accessor methods - public for testing purposes
    Player& GetPlayer();
    CollisionManager& GetCollisionManager();
    ModelLoader& GetModels();
    WorldManager& GetWorld();
    Menu& GetMenu();
    GameMap& GetGameMap();
    bool IsInitialized() const;
};

#endif // GAME_H