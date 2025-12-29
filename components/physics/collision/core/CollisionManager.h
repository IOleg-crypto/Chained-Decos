#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include "components/physics/collision/colsystem/CollisionSystem.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"

#include "scene/ecs/Entity.h"
#include <algorithm>
#include <array>
#include <execution>
#include <future>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "components/physics/collision/core/CollisionModelProcessor.h"
#include "components/physics/collision/core/CollisionPredictionCache.h"
#include "components/physics/collision/core/CollisionSpatialGrid.h"

// Include ModelLoader header
#include "scene/resources/model/Model.h"

// Structure to hold model processing data for parallel processing
struct ModelCollisionTask
{
    std::string modelName;
    Model *model;
    bool hasCollision;
    std::vector<ModelInstance *> instances;
    int createdCollisions = 0;
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

    // Create collisions only for specific models
    void CreateAutoCollisionsFromModelsSelective(ModelLoader &models,
                                                 const std::vector<std::string> &modelNames);

    // Helper function to create cache key
    [[nodiscard]] std::string MakeCollisionCacheKey(const std::string &modelName,
                                                    float scale) const;

    // Create collision for a specific model instance
    bool CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                  Vector3 position, float scale, const ModelLoader &models);

    // Prediction cache management
    void UpdateFrameCache();
    void ClearExpiredCache();

private:
    std::vector<std::shared_ptr<Collision>> m_collisionObjects;

    // Extracted helper classes
    CollisionSpatialGrid m_grid;
    CollisionModelProcessor m_modelProcessor;
    CollisionPredictionCache m_cache;

    // Cache to prevent rebuilding precise collisions for same models
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;

    // Limit the number of precise collisions per model
    std::unordered_map<std::string, int> m_preciseCollisionCountPerModel;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50;

    // Dynamic Entity Storage
    std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> m_entityColliders;

    size_t m_currentFrame = 0;
};

#endif // COLLISIONMANAGER_H
