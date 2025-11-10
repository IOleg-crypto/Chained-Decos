#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include <string>
#include <vector>
#include "Engine/Map/Core/MapLoader.h"
#include "Engine/Collision/Manager/CollisionManager.h"
#include "Engine/Model/Core/Model.h"

// Forward declarations
class Player;

// MapCollisionInitializer - handles collision initialization for maps
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(CollisionManager* collisionManager, ModelLoader* models, Player* player = nullptr);
    ~MapCollisionInitializer() = default;

    // Initialize collisions for map objects
    void InitializeCollisions(const GameMap& gameMap);

    // Initialize collisions with specific models
    void InitializeCollisionsWithModels(const GameMap& gameMap, const std::vector<std::string>& requiredModels);

    // Initialize collisions safely (doesn't fail if models missing)
    bool InitializeCollisionsWithModelsSafe(const GameMap& gameMap, const std::vector<std::string>& requiredModels);

    // Set player reference (used when PlayerSystem initializes after MapSystem)
    void SetPlayer(Player* player);

private:
    CollisionManager* m_collisionManager;
    ModelLoader* m_models;
    Player* m_player;
};

#endif // MAP_COLLISION_INITIALIZER_H

