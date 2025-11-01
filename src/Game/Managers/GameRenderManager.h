#ifndef GAME_RENDER_MANAGER_H
#define GAME_RENDER_MANAGER_H

#include "../Player/Player.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/Collision/CollisionManager.h"
#include "MapManager.h"

class GameRenderManager
{
private:
    Player* m_player;
    Engine* m_engine;
    ModelLoader* m_models;
    CollisionManager* m_collisionManager;
    MapManager* m_mapManager;

public:
    GameRenderManager(Player* player, Engine* engine, ModelLoader* models,
                      CollisionManager* collisionManager, MapManager* mapManager);
    ~GameRenderManager() = default;

    void RenderGameWorld();
    void RenderGameUI() const;
};

#endif // GAME_RENDER_MANAGER_H

