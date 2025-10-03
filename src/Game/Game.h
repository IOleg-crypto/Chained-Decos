#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Menu/Menu.h"
#include "Player/Player.h"

class Game
{
private:
    Player m_player;
    CollisionManager m_collisionManager;
    ModelLoader m_models;
    WorldManager m_world;
    Menu m_menu;
    Engine *m_engine = nullptr;

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
    void RenderGameWorld() const;
    void RenderGameUI() const;
    
    // Parkour test map creation
    void CreateParkourTestMap();
};

#endif // GAME_H