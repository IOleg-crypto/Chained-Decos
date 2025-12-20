#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include "components/physics/collision/core/collisionManager.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/model/core/Model.h"
#include <string>
#include <vector>

// Handles collision initialization for GameMap objects
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(std::shared_ptr<CollisionManager> collisionManager,
                            std::shared_ptr<ModelLoader> models,
                            std::shared_ptr<IPlayer> player = nullptr);
    ~MapCollisionInitializer() = default;

    void InitializeCollisions(const GameMap &gameMap);
    void InitializeCollisionsWithModels(const GameMap &gameMap,
                                        const std::vector<std::string> &requiredModels);
    bool InitializeCollisionsWithModelsSafe(const GameMap &gameMap,
                                            const std::vector<std::string> &requiredModels);

    void SetPlayer(std::shared_ptr<IPlayer> player);

private:
    std::shared_ptr<CollisionManager> m_collisionManager;
    std::shared_ptr<ModelLoader> m_models;
    std::shared_ptr<IPlayer> m_player;
};

#endif // MAP_COLLISION_INITIALIZER_H


