#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Menu/Menu.h"
#include "Engine/Model/Model.h"
#include "Player/Player.h"

class Game
{
private:
    Engine &m_engine;
    Player m_player;
    CollisionManager m_collisionManager;
    Models m_models;
    Menu m_menu;

    // Стани гри
    bool m_showMenu;
    bool m_isGameInitialized;

public:
    Game(Engine &engine);
    ~Game();

    void Init();
    void Run();

    void ToggleMenu();
    void RequestExit();
    bool IsRunning() const;

private:
    void Update();
    void Render();
    void InitInput();
    void InitCollisions();
    void InitPlayer();
    void LoadGameModels();
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    void HandleKeyboardShortcuts();
    void HandleMenuActions();
    void RenderGameWorld();
    void RenderGameUI();
};

#endif // GAME_H