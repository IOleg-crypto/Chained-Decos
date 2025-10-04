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
    Player m_player;
    CollisionManager m_collisionManager;
    ModelLoader m_models;
    WorldManager m_world;
    Menu m_menu;
    Engine *m_engine = nullptr;

    // Map loading system
    std::vector<MapLoader> m_mapObjects;  // Legacy support
    GameMap m_gameMap;                   // New comprehensive map system

private:
    bool m_showMenu;
    bool m_isGameInitialized;
    bool m_isDebugInfo;

public:
    Game(Engine *engine);
    ~Game();

    void Init();
    void Run();

    void ToggleMenu();
    void RequestExit() const;
    bool IsRunning() const;

    void Update();
    void Render();
    void InitInput();
    void InitCollisions();
    void InitPlayer();
    void LoadGameModels();
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    //void HandleKeyboardShortcuts(); maybe implemented in the future
    void HandleMenuActions();
    void RenderGameWorld();
    void RenderGameUI() const;
    
    // Parkour test map creation
    void CreateParkourTestMap();
    void CreateEasyParkourMap();
    void CreateMediumParkourMap();
    void CreateHardParkourMap();
    void CreateSpeedrunParkourMap();

    // Helper function for simplified platform creation
    void AddPlatform(float x, float y, float z, float sizeX, float sizeY, float sizeZ);

    // Map loading and rendering
    void LoadEditorMap(const std::string& mapPath);
    void RenderEditorMap();
};

#endif // GAME_H