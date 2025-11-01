#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <raylib.h>
#include "Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Engine.h"
#include "MapManager.h"

class PlayerManager
{
private:
    static constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 2.0f;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    Engine* m_engine;
    MapManager* m_mapManager;

public:
    PlayerManager(Player* player, CollisionManager* collisionManager, 
                  ModelLoader* models, Engine* engine, MapManager* mapManager);
    ~PlayerManager() = default;

    void InitPlayer();
    void UpdatePlayerLogic();
};

#endif // PLAYER_MANAGER_H

