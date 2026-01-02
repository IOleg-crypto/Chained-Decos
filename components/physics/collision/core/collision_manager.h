#ifndef CD_COMPONENTS_PHYSICS_COLLISION_CORE_COLLISION_MANAGER_H
#define CD_COMPONENTS_PHYSICS_COLLISION_CORE_COLLISION_MANAGER_H

#include "components/physics/collision/colsystem/collision_system.h"

#include "scene/ecs/entity.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <set>

// Include ModelLoader header
#include "scene/resources/model/model.h"
using namespace ECS;

namespace CHEngine
{

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
class CollisionManager
{
public:
    static void Init();
    static bool Initialize()
    {
        Init();
        return true;
    }
    static void Shutdown();
    static bool IsInitialized();
    static void Update(float deltaTime);
    static void Render();

    // Spatial partitioning optimized collision queries
    static void UpdateSpatialPartitioning();

    // Add a new collider to the manager
    static void AddCollider(std::shared_ptr<Collision> collider);

    // Remove all colliders
    static void ClearColliders();

    // Check if a collision intersects with any collider
    static bool CheckCollision(const Collision &playerCollision);

    // Spatial partitioning optimized collision checking
    static bool CheckCollisionSpatial(const Collision &playerCollision);

    // Check collision and provide collision response vector
    static bool CheckCollision(const Collision &playerCollision, Vector3 &response);

    // Get all colliders
    static const std::vector<std::shared_ptr<Collision>> &GetColliders();

    // Raycast down against precise colliders (BVH or triangle) to find ground beneath a point
    static bool RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                            Vector3 &hitPoint, Vector3 &hitNormal);

    // Dynamic Entity Management (ECS Integration)
    static void AddEntityCollider(ECS::EntityID entity, const std::shared_ptr<Collision> &collider);
    static void RemoveEntityCollider(ECS::EntityID entity);
    static void UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position);
    static std::shared_ptr<Collision> GetEntityCollider(ECS::EntityID entity);

    // Check collision against all entities (excluding self)
    static bool CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                     std::vector<ECS::EntityID> &outCollidedEntities);

    // Create collision for a specific model instance
    static bool CreateCollisionFromModel(const ::Model &model, const std::string &modelName,
                                         Vector3 position, float scale, const ModelLoader &models);

    CollisionManager();
    ~CollisionManager();

private:
    // Internal implementations
    bool InternalInitialize();
    void InternalUpdateSpatialPartitioning();
    void InternalAddCollider(std::shared_ptr<Collision> collider);
    void InternalClearColliders();
    bool InternalCheckCollision(const Collision &playerCollision) const;
    bool InternalCheckCollisionSpatial(const Collision &playerCollision) const;
    bool InternalCheckCollision(const Collision &playerCollision, Vector3 &response) const;
    const std::vector<std::shared_ptr<Collision>> &InternalGetColliders() const
    {
        return m_collisionObjects;
    }
    bool InternalRaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                             Vector3 &hitPoint, Vector3 &hitNormal) const;
    void InternalAddEntityCollider(ECS::EntityID entity,
                                   const std::shared_ptr<Collision> &collider);
    void InternalRemoveEntityCollider(ECS::EntityID entity);
    void InternalUpdateEntityCollider(ECS::EntityID entity, const Vector3 &position);
    std::shared_ptr<Collision> InternalGetEntityCollider(ECS::EntityID entity) const;
    bool InternalCheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                      std::vector<ECS::EntityID> &outCollidedEntities) const;
    bool InternalCreateCollisionFromModel(const ::Model &model, const std::string &modelName,
                                          Vector3 position, float scale, const ModelLoader &models);

    // Helper functions
    std::string MakeCollisionCacheKey(const std::string &modelName, float scale) const;
    void BuildSpatialGrid(const std::vector<std::shared_ptr<Collision>> &objects);
    void BuildEntityGrid(
        const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entityColliders);
    std::vector<size_t> GetNearbyObjectIndices(const Collision &target) const;
    std::vector<ECS::EntityID> GetNearbyEntities(const Collision &target) const;

    std::shared_ptr<Collision> CreateBaseCollision(const ::Model &model,
                                                   const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);
    Collision CreatePreciseInstanceCollision(const ::Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);
    Collision CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                       Vector3 position, float scale);
    Collision CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                const Vector3 &position, float scale);

private:
    std::vector<std::shared_ptr<Collision>> m_collisionObjects;

    // Spatial Partitioning
    float m_cellSize = 10.0f;
    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_staticGrid;
    std::unordered_map<GridKey, std::vector<ECS::EntityID>, GridKeyHash> m_entityGrid;

    // Model Processing
    CollisionConfig m_config;
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;
    std::unordered_map<std::string, int> m_preciseCollisionCountPerModel;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50;

    // Dynamic Entity Storage
    std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> m_entityColliders;
};

} // namespace CHEngine

#endif // CD_COMPONENTS_PHYSICS_COLLISION_CORE_COLLISION_MANAGER_H
