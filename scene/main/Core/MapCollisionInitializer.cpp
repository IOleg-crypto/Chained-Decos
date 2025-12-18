#include "MapCollisionInitializer.h"

#include "core/ecs/Components/PhysicsData.h"
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
    // Only clear existing colliders if no custom map is loaded
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        m_collisionManager->ClearColliders();
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();

    // Load model collisions only for models that are actually loaded and required for this map
    auto availableModels = m_models->GetAvailableModels();
    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, availableModels);

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player)
    {
        m_player->InitializeCollision();
    }
}

void MapCollisionInitializer::InitializeCollisionsWithModels(
    const GameMap &gameMap, const std::vector<std::string> &requiredModels)
{
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        m_collisionManager->ClearColliders();
    }

    m_collisionManager->Initialize();
    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    m_collisionManager->Initialize();

    if (m_player)
    {
        m_player->InitializeCollision();
    }
}

bool MapCollisionInitializer::InitializeCollisionsWithModelsSafe(
    const GameMap &gameMap, const std::vector<std::string> &requiredModels)
{
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        m_collisionManager->ClearColliders();
    }

    m_collisionManager->Initialize();
    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    m_collisionManager->Initialize();

    if (m_player)
    {
        m_player->InitializeCollision();
    }

    // Sync collisions to ECS
    for (const auto &collider : m_collisionManager->GetColliders())
    {
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

    return true;
}

void MapCollisionInitializer::SetPlayer(IPlayer *player)
{
    m_player = player;
}
