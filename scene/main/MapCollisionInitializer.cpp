#include "MapCollisionInitializer.h"

#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/TransformComponent.h"
#include <raylib.h>

namespace CHEngine
{

MapCollisionInitializer::MapCollisionInitializer(ModelLoader *models,
                                                 std::shared_ptr<::IPlayer> player)
    : m_models(models), m_player(std::move(player))
{
}

void MapCollisionInitializer::InitializeCollisions(entt::registry &registry,
                                                   const GameScene &gameMap)
{
    // Only clear existing colliders if no custom map is loaded
    size_t previousColliderCount = CollisionManager::GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        CollisionManager::ClearColliders();
    }

    // Initialize ground collider first
    CollisionManager::Initialize();

    // Reinitialize after adding all model colliders
    CollisionManager::Initialize();

    // Initialize player collision (if player is available)
    if (m_player)
    {
        m_player->InitializeCollision();
    }

    // 4. Initialize primitive collisions (CUBE, PLANE)
    for (const auto &obj : gameMap.GetMapObjects())
    {
        if (obj.type == MapObjectType::CUBE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            Vector3 halfSize = {obj.scale.x * 0.5f, obj.scale.y * 0.5f, obj.scale.z * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
        else if (obj.type == MapObjectType::PLANE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            // Planes need some thickness to prevent tunneling
            Vector3 halfSize = {obj.size.x * 0.5f, 0.1f, obj.size.y * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
    }
}

void MapCollisionInitializer::InitializeCollisionsWithModels(
    entt::registry &registry, const GameScene &gameMap,
    const std::vector<std::string> &requiredModels)
{
    size_t previousColliderCount = CollisionManager::GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        CollisionManager::ClearColliders();
    }

    CollisionManager::Initialize();
    CollisionManager::Initialize();

    if (m_player)
    {
        m_player->InitializeCollision();
    }

    // 4. Initialize primitive collisions (CUBE, PLANE)
    for (const auto &obj : gameMap.GetMapObjects())
    {
        if (obj.type == MapObjectType::CUBE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            Vector3 halfSize = {obj.scale.x * 0.5f, obj.scale.y * 0.5f, obj.scale.z * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
        else if (obj.type == MapObjectType::PLANE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            Vector3 halfSize = {obj.size.x * 0.5f, 0.1f, obj.size.y * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
    }
}

bool MapCollisionInitializer::InitializeCollisionsWithModelsSafe(
    entt::registry &registry, const GameScene &gameMap,
    const std::vector<std::string> &requiredModels)
{
    size_t previousColliderCount = CollisionManager::GetColliders().size();
    if (previousColliderCount > 0 && gameMap.GetMapObjects().empty())
    {
        CollisionManager::ClearColliders();
    }

    CollisionManager::Initialize();
    CollisionManager::Initialize();

    if (m_player)
    {
        m_player->InitializeCollision();
    }

    // Initialize primitive collisions (CUBE, PLANE)
    for (const auto &obj : gameMap.GetMapObjects())
    {
        if (obj.type == MapObjectType::CUBE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            Vector3 halfSize = {obj.scale.x * 0.5f, obj.scale.y * 0.5f, obj.scale.z * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
        else if (obj.type == MapObjectType::PLANE && (obj.isPlatform || obj.isObstacle))
        {
            Vector3 center = obj.position;
            Vector3 halfSize = {obj.size.x * 0.5f, 0.1f, obj.size.y * 0.5f};
            auto col = std::make_shared<Collision>(center, halfSize);
            CollisionManager::AddCollider(col);
        }
    }

    // Sync collisions to ECS
    for (auto &&collider : CollisionManager::GetColliders())
    {
        auto entity = registry.create();
        registry.emplace<TransformComponent>(entity, collider->GetCenter(), Vector3{0, 0, 0},
                                             Vector3{1, 1, 1});

        auto &colComp = registry.emplace<CollisionComponent>(entity);
        colComp.bounds = collider->GetBoundingBox();
        colComp.collider = collider;
        colComp.isTrigger = false;
        colComp.collisionLayer = 1; // Default layer
        colComp.collisionMask = ~0;
        colComp.hasCollision = false;
    }

    return true;
}

void MapCollisionInitializer::SetPlayer(std::shared_ptr<::IPlayer> player)
{
    m_player = std::move(player);
}

} // namespace CHEngine
