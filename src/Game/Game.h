#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Map/MapLoader.h" // For loading editor-created maps
#include "Menu/Menu.h"
#include "Player/Player.h"
#include <memory>

class Game

{
private:
    float PLAYER_SAFE_SPAWN_HEIGHT = 2.0f;
    static Game *s_instance;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<CollisionManager> m_collisionManager;
    std::unique_ptr<ModelLoader> m_models;
    std::unique_ptr<WorldManager> m_world;
    std::unique_ptr<Menu> m_menu;
    std::unique_ptr<Engine> m_engine;

    // Map loading system
    GameMap m_gameMap; // New comprehensive map system

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
    Game();
    ~Game();

    void Init(int argc, char *argv[]);
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
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);
    void InitPlayer();
    std::optional<ModelLoader::LoadResult> LoadGameModels();
    std::optional<ModelLoader::LoadResult>
    LoadGameModelsSelective(const std::vector<std::string> &modelNames);
    std::optional<ModelLoader::LoadResult>
    LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);
    std::string GetModelNameForObjectType(int objectType, const std::string &modelName = "");
    std::vector<std::string> GetModelsRequiredForMap(const std::string &mapIdentifier);
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    void Cleanup(); // Resource cleanup
    // void HandleKeyboardShortcuts(); maybe implemented in the future
    void HandleMenuActions();
    void RenderGameWorld();
    void RenderGameUI() const;

    // Helper functions for code quality and reduced duplication
    void CreatePlatform(const Vector3 &position, const Vector3 &size, Color color,
                        CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize);

    // Map loading and rendering
    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    // Diagnostics: dump map vs model registry for debugging missing instances
    void DumpMapDiagnostics() const;

    // Game state management
    void SaveGameState();
    void RestoreGameState();

    // Singleton methods
    static Game *GetInstance();

    // Test accessor methods - public for testing purposes
    Player &GetPlayer() { return *m_player; }
    CollisionManager &GetCollisionManager() { return *m_collisionManager; }
    ModelLoader &GetModels() { return *m_models; }
    WorldManager &GetWorld() { return *m_world; }
    Menu &GetMenu() { return *m_menu; }
    GameMap &GetGameMap();
    bool IsInitialized() const;
};

#endif // GAME_H