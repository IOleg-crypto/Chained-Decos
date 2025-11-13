#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <raylib.h>

class Player;
class CollisionManager;
class ModelLoader;
class Engine;
class MapManager;
class AudioManager;

// Manages player initialization and update logic
// Single Responsibility: Only handles player lifecycle
class PlayerManager
{
private:
    static constexpr float PLAYER_SAFE_SPAWN_HEIGHT = 1.5f;
    
    Player* m_player;
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    Engine* m_engine;
    MapManager* m_mapManager;
    AudioManager* m_audioManager;

public:
    PlayerManager(Player* player, CollisionManager* collisionManager, 
                  ModelLoader* models, Engine* engine, MapManager* mapManager, AudioManager* audioManager);
    ~PlayerManager() = default;

    void InitPlayer();
    void UpdatePlayerLogic();
};

#endif // PLAYER_MANAGER_H

