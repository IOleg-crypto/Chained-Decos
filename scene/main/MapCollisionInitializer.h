#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include "components/physics/collision/core/collisionManager.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/model/Model.h"
#include <string>
#include <vector>

// Handles collision initialization for GameScene objects
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(CollisionManager *collisionManager, ModelLoader *models,
                            std::shared_ptr<IPlayer> player = nullptr);
    ~MapCollisionInitializer() = default;

    void InitializeCollisions(const GameScene &gameMap);
    void InitializeCollisionsWithModels(const GameScene &gameMap,
                                        const std::vector<std::string> &requiredModels);
    bool InitializeCollisionsWithModelsSafe(const GameScene &gameMap,
                                            const std::vector<std::string> &requiredModels);

    void SetPlayer(std::shared_ptr<IPlayer> player);

private:
    CollisionManager *m_collisionManager;
    ModelLoader *m_models;
    std::shared_ptr<IPlayer> m_player;
};

#endif // MAP_COLLISION_INITIALIZER_H
