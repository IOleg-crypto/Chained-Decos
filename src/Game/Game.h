#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Game/Menu/Menu.h"
#include "Model/Model.h"
#include "Player/Player.h"

class Engine;

class Game
{
private:
    Engine &m_engine;
    Player m_player;
    CollisionManager m_collisionManager;
    ModelLoader m_models;
    Menu m_menu;
private:
    bool m_showMenu;
    bool m_isGameInitialized;
    bool m_isDebugInfo;

public:
    explicit Game(Engine &engine);
    ~Game();

    void Init();
    void Run();

    void ToggleMenu();
    void RequestExit() const;
    bool IsRunning() const;

private:
    void Update();
    void Render();
    void InitInput();
    void InitCollisions();
    void InitPlayer() const;
    void LoadGameModels();
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();
    //void HandleKeyboardShortcuts(); maybe implemented in the future
    void HandleMenuActions();
    void RenderGameWorld() const;
    void RenderGameUI() const;
};

#endif // GAME_H