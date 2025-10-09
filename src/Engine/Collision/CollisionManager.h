#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include "CollisionSystem.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <Model/ModelConfig.h>
#include <algorithm>
#include <execution>
#include <future>
#include <array>


class ModelLoader;

//
// CollisionManager
// Manages all collision boxes in the game.
// Supports adding, clearing, and checking collisions.
// Uses AABB for fast checks and optional BVH for precise collisions.
//
class CollisionManager
{
public:
    CollisionManager() = default;

    // Initialize the collision system
    void Initialize() const;

    // Update spatial partitioning for optimized collision queries
    void UpdateSpatialPartitioning();

    // Add a new collider to the manager
    void AddCollider(Collision &&collider);
    
    // Add an existing collider reference
    void AddColliderRef(Collision* collider);

    // Remove all colliders
    void ClearColliders();

    // Check if a collision intersects with any collider
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision) const;

    // Spatial partitioning optimized collision checking
    [[nodiscard]] bool CheckCollisionSpatial(const Collision &playerCollision) const;

    // Check collision and provide collision response vector
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision, Vector3 &response) const;

    // Parallel version of collision checking for better performance with many objects
    [[nodiscard]] bool CheckCollisionParallel(const Collision &playerCollision) const;

    // Parallel version with response vector
    [[nodiscard]] bool CheckCollisionParallel(const Collision &playerCollision, Vector3 &response) const;

    // Get all colliders
    [[nodiscard]] const std::vector<std::unique_ptr<Collision>> &GetColliders() const;

    // Raycast down against precise colliders (BVH or triangle) to find ground beneath a point
    bool RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                     Vector3 &hitPoint, Vector3 &hitNormal) const;

    // Automatically create collisions for all models
    void CreateAutoCollisionsFromModels(ModelLoader &models);

    // Helper function to create cache key
    [[nodiscard]] std::string MakeCollisionCacheKey(const std::string &modelName,
                                                    float scale) const;

    // Create collision for a specific model instance
    bool CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                  Vector3 position, float scale, const ModelLoader &models);

    // Create a base collision for caching (AABB or BVH)
    std::shared_ptr<Collision> CreateBaseCollision(const Model &model, const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);

    // Create precise collision (Triangle/BVH) for an instance
    Collision CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);

    // Create precise collision from cached triangles to avoid re-reading model meshes
    Collision CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                       Vector3 position, float scale);

    // Create simple AABB collision for an instance
    Collision CreateSimpleAABBInstanceCollision(
        const Collision &cachedCollision,  const Vector3 &position, float scale);

    // Helper method to check collision with a single object (for parallel processing)
    bool CheckCollisionSingleObject(const Collision &playerCollision,
                                    const Collision &collisionObject,
                                    Vector3 &response) const;

private:
    std::vector<std::unique_ptr<Collision>> m_collisionObjects; // All collision objects

    // Cache to prevent rebuilding precise collisions for same models
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;

    // Limit the number of precise collisions per model
    std::unordered_map<std::string, int> m_preciseCollisionCountPerModel;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50;

    // Spatial partitioning for faster collision queries
    struct GridKey {
        int x, z;
        bool operator==(const GridKey& other) const {
            return x == other.x && z == other.z;
        }
    };

    struct GridKeyHash {
        std::size_t operator()(const GridKey& key) const {
            return std::hash<int>()(key.x) ^ (std::hash<int>()(key.z) << 1);
        }
    };

    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_spatialGrid;
};

#endif // COLLISIONMANAGER_H
