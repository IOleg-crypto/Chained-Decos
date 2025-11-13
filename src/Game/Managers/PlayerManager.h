#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <raylib.h>
#include "../Player/Player.h"
#include "Engine/Collision/Manager/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include "Engine/Engine.h"
#include "MapManager.h"
#include "Engine/Audio/Core/AudioManager.h"

class PlayerManager
{
private:
    static constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 1.5f;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    Engine* m_engine;
    MapManager* m_mapManager;
    AudioManager* m_audioManager = nullptr;

public:
    PlayerManager(Player* player, CollisionManager* collisionManager, 
                  ModelLoader* models, Engine* engine, MapManager* mapManager, AudioManager* audioManager);
    ~PlayerManager() = default;

    void InitPlayer();
    void UpdatePlayerLogic();
};

#endif // PLAYER_MANAGER_H

