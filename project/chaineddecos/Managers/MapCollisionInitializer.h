#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include <string>
#include <vector>
#include "scene/resources/map/Core/MapLoader.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include "scene/resources/model/Core/Model.h"

#include "core/interfaces/IPlayer.h"

// MapCollisionInitializer - handles collision initialization for maps
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(CollisionManager* collisionManager, ModelLoader* models, IPlayer* player = nullptr);
    ~MapCollisionInitializer() = default;

    // Initialize collisions for map objects
    void InitializeCollisions(const GameMap& gameMap);

    // Initialize collisions with specific models
    void InitializeCollisionsWithModels(const GameMap& gameMap, const std::vector<std::string>& requiredModels);

    // Initialize collisions safely (doesn't fail if models missing)
    bool InitializeCollisionsWithModelsSafe(const GameMap& gameMap, const std::vector<std::string>& requiredModels);

    // Set player reference (used when PlayerSystem initializes after MapSystem)
    void SetPlayer(IPlayer* player);

private:
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    IPlayer* m_player;
};

#endif // MAP_COLLISION_INITIALIZER_H

