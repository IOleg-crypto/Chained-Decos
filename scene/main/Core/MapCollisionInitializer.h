#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include "components/physics/collision/Core/CollisionManager.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/Core/MapLoader.h"
#include "scene/resources/model/Core/Model.h"
#include <string>
#include <vector>

// Handles collision initialization for GameMap objects
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(CollisionManager *collisionManager, ModelLoader *models,
                            IPlayer *player = nullptr);
    ~MapCollisionInitializer() = default;

    void InitializeCollisions(const GameMap &gameMap);
    void InitializeCollisionsWithModels(const GameMap &gameMap,
                                        const std::vector<std::string> &requiredModels);
    bool InitializeCollisionsWithModelsSafe(const GameMap &gameMap,
                                            const std::vector<std::string> &requiredModels);

    void SetPlayer(IPlayer *player);

private:
    CollisionManager *m_collisionManager;
    ModelLoader *m_models;
    IPlayer *m_player;
};

#endif // MAP_COLLISION_INITIALIZER_H
