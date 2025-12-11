#include "MapCollisionInitializer.h"

#include "core/ecs/Components/PhysicsComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/ECSRegistry.h"
#include <raylib.h>

MapCollisionInitializer::MapCollisionInitializer(CollisionManager *collisionManager,
                                                 ModelLoader *models, IPlayer *player)
    : m_collisionManager(collisionManager), m_models(models), m_player(player)
{
}

void MapCollisionInitializer::InitializeCollisions(const GameMap &gameMap)
{
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisions() - Initializing collision system...");

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisions() - Clearing %zu existing "
                 "colliders (no map loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisions() - Map loaded with %zu existing "
                 "colliders, preserving them",
                 previousColliderCount);
    }

    // Ground is now provided by map objects, no artificial ground needed
    if (gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO, "MapCollisionInitializer::InitializeCollisions() - No custom map "
                           "loaded, no ground will be created");
    }
    else
    {
        TraceLog(LOG_INFO, "MapCollisionInitializer::InitializeCollisions() - Custom map loaded, "
                           "using map's ground objects");
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();

    // Load model collisions only for models that are actually loaded and required for this map
    auto availableModels = m_models->GetAvailableModels();
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisions() - Available models for collision "
             "generation: %d",
             availableModels.size());
    for (const auto &modelName : availableModels)
    {
        TraceLog(LOG_INFO, "MapCollisionInitializer::InitializeCollisions() - Model available: %s",
                 modelName.c_str());
    }

    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, availableModels);
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisions() - Model collisions created");

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player)
    {
        m_player->InitializeCollision();
    }
    else
    {
        TraceLog(LOG_WARNING, "MapCollisionInitializer::InitializeCollisions() - Player not "
                              "available, skipping player collision initialization");
    }

    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisions() - Collision system initialized with "
             "%zu colliders.",
             m_collisionManager->GetColliders().size());
}

void MapCollisionInitializer::InitializeCollisionsWithModels(
    const GameMap &gameMap, const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModels() - Initializing collision "
             "system with %d required "
             "models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisionsWithModels() - Clearing %zu "
                 "existing colliders (no map loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisionsWithModels() - Map loaded with %zu "
                 "existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModels() - Required models for "
             "collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisionsWithModels() - Model required: %s",
                 modelName.c_str());
    }

    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    TraceLog(
        LOG_INFO,
        "MapCollisionInitializer::InitializeCollisionsWithModels() - Model collisions created");

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player)
    {
        m_player->InitializeCollision();
    }
    else
    {
        TraceLog(LOG_WARNING, "MapCollisionInitializer::InitializeCollisionsWithModels() - Player "
                              "not available, skipping player collision initialization");
    }

    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModels() - Collision system "
             "initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());
}

bool MapCollisionInitializer::InitializeCollisionsWithModelsSafe(
    const GameMap &gameMap, const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Initializing "
             "collision system with %d "
             "required models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Clearing %zu "
                 "existing colliders (no map "
                 "loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Map loaded with "
                 "%zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize collision manager
    m_collisionManager->Initialize();

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Required models for "
             "collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(
            LOG_INFO,
            "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Model required: %s",
            modelName.c_str());
    }

    // Try to create model collisions safely
    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    TraceLog(
        LOG_INFO,
        "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Model collisions created");

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player)
    {
        m_player->InitializeCollision();
    }
    else
    {
        TraceLog(LOG_WARNING, "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - "
                              "Player not available, skipping player collision initialization");
    }

    TraceLog(LOG_INFO,
             "MapCollisionInitializer::InitializeCollisionsWithModelsSafe() - Collision system "
             "initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());

    // Sync collisions to ECS
    for (const auto &collider : m_collisionManager->GetColliders())
    {
        // Simple check to avoid duplicating if entity already exists?
        // Ideally we should clear old map entities first.
        // But for now, let's just create them.
        auto entity = REGISTRY.create();
        REGISTRY.emplace<TransformComponent>(entity, collider->GetCenter(), Vector3{0, 0, 0},
                                             Vector3{1, 1, 1});

        auto &colComp = REGISTRY.emplace<CollisionComponent>(entity);
        colComp.bounds = collider->GetBoundingBox();
        colComp.collider = collider;
        colComp.isTrigger = false;
        colComp.collisionLayer = 1; // Default layer
        colComp.collisionMask = ~0;
        colComp.hasCollision = false;
    }

    return true; // Always return true since we have at least basic collision
}

void MapCollisionInitializer::SetPlayer(IPlayer *player)
{
    m_player = player;
    TraceLog(LOG_INFO, "MapCollisionInitializer::SetPlayer() - Player reference updated");
}
