#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include "components/physics/collision/colsystem/CollisionSystem.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"

#include "scene/ecs/Entity.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <set>

// Include ModelLoader header
#include "scene/resources/model/Model.h"

struct CollisionConfig
{
    int maxPrecisePerModel = 50;
};

//
// CollisionManager
// Manages all collision boxes in the game.
// Supports adding, clearing, and checking collisions.
// Uses AABB for fast checks and optional BVH for precise collisions.
//
class CollisionManager : public ICollisionManager
{
public:
    CollisionManager() = default;

    // Initialize the collision system
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime)
    {
    }
    void Render();

    // Update spatial partitioning for optimized collision queries
    void UpdateSpatialPartitioning();

    // Add a new collider to the manager
    void AddCollider(std::shared_ptr<Collision> collider) override;

    // Remove all colliders
    void ClearColliders() override;

    // Check if a collision intersects with any collider
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision) const override;

    // Spatial partitioning optimized collision checking
    [[nodiscard]] bool CheckCollisionSpatial(const Collision &playerCollision) const;

    // Check collision and provide collision response vector
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision,
                                      Vector3 &response) const override;

    // Get all colliders
    [[nodiscard]] const std::vector<std::shared_ptr<Collision>> &GetColliders() const override;

    // Raycast down against precise colliders (BVH or triangle) to find ground beneath a point
    bool RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                     Vector3 &hitPoint, Vector3 &hitNormal) const override;

    // Dynamic Entity Management (ECS Integration)
    void AddEntityCollider(ECS::EntityID entity,
                           const std::shared_ptr<Collision> &collider) override;
    void RemoveEntityCollider(ECS::EntityID entity) override;
    void UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position) override;
    [[nodiscard]] std::shared_ptr<Collision> GetEntityCollider(ECS::EntityID entity) const override;

    // Check collision against all entities (excluding self)
    [[nodiscard]] bool
    CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                         std::vector<ECS::EntityID> &outCollidedEntities) const override;

    // Helper function to create cache key
    [[nodiscard]] std::string MakeCollisionCacheKey(const std::string &modelName,
                                                    float scale) const;

    // Create collision for a specific model instance
    bool CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                  Vector3 position, float scale, const ModelLoader &models);

private:
    std::vector<std::shared_ptr<Collision>> m_collisionObjects;

    // Spatial Partitioning
    float m_cellSize = 10.0f;
    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_staticGrid;
    std::unordered_map<GridKey, std::vector<ECS::EntityID>, GridKeyHash> m_entityGrid;

    void BuildSpatialGrid(const std::vector<std::shared_ptr<Collision>> &objects);
    void BuildEntityGrid(
        const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entityColliders);
    [[nodiscard]] std::vector<size_t> GetNearbyObjectIndices(const Collision &target) const;
    [[nodiscard]] std::vector<ECS::EntityID> GetNearbyEntities(const Collision &target) const;

    // Model Processing
    CollisionConfig m_config;
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;
    std::unordered_map<std::string, int> m_preciseCollisionCountPerModel;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50;

    std::shared_ptr<Collision> CreateBaseCollision(const Model &model, const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);

    Collision CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);

    Collision CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                       Vector3 position, float scale);

    Collision CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                const Vector3 &position, float scale);

    // Dynamic Entity Storage
    std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> m_entityColliders;
};

#endif // COLLISIONMANAGER_H
