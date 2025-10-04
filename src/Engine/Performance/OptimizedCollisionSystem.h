#ifndef OPTIMIZED_COLLISION_SYSTEM_H
#define OPTIMIZED_COLLISION_SYSTEM_H

#include <vector>
#include <array>
#include <unordered_set>
#include <raylib.h>
#include "../Collision/CollisionStructures.h"

struct SpatialGridCell {
    std::vector<Collision*> colliders;
    Vector3 boundsMin;
    Vector3 boundsMax;

    SpatialGridCell(const Vector3& min, const Vector3& max) : boundsMin(min), boundsMax(max) {}
    void AddCollider(Collision* collider) { colliders.push_back(collider); }
    void RemoveCollider(Collision* collider);
    void Clear() { colliders.clear(); }
};

class SpatialGrid {
public:
    SpatialGrid(const Vector3& gridMin, const Vector3& gridMax, float cellSize);
    ~SpatialGrid() = default;

    // Grid management
    void Update(const std::vector<std::unique_ptr<Collision>>& colliders);
    void Clear();

    // Query methods
    std::vector<Collision*> GetNearbyColliders(const Vector3& position, float radius);
    std::vector<Collision*> GetCollidersInCell(int x, int y, int z);
    std::vector<Collision*> GetCollidersInAABB(const BoundingBox& aabb);

    // Optimization
    void OptimizeGridSize(float newCellSize);
    void SetAutoOptimization(bool enable) { m_autoOptimize = enable; }

private:
    Vector3 m_gridMin;
    Vector3 m_gridMax;
    float m_cellSize;
    Vector3 m_gridSize;
    std::array<int, 3> m_gridDimensions;

    std::vector<std::vector<std::vector<SpatialGridCell>>> m_cells;
    bool m_autoOptimize;

    // Helper methods
    std::array<int, 3> GetCellIndex(const Vector3& position);
    bool IsValidCell(int x, int y, int z) const;
    void ExpandGridIfNeeded(const Vector3& position);
    BoundingBox GetCellBounds(int x, int y, int z) const;
};

struct CollisionPair {
    Collision* colliderA;
    Collision* colliderB;
    float distance;

    CollisionPair(Collision* a, Collision* b, float dist) : colliderA(a), colliderB(b), distance(dist) {}
};

class OptimizedCollisionSystem {
public:
    OptimizedCollisionSystem();
    ~OptimizedCollisionSystem() = default;

    // System management
    void Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Collision detection
    bool CheckCollision(const Collision& collision, Vector3& response);
    std::vector<CollisionPair> GetNearbyCollisions(const Vector3& position, float radius);

    // Broad phase optimization
    void EnableSpatialGrid(bool enable);
    void SetSpatialGridBounds(const Vector3& min, const Vector3& max);
    void SetSpatialGridCellSize(float cellSize);

    // Performance settings
    void SetMaxCollisionChecks(int maxChecks);
    void EnableCollisionCaching(bool enable);
    void SetCollisionDistanceThreshold(float threshold);

    // Statistics
    struct CollisionStats {
        int totalChecks;
        int actualChecks;
        int cacheHits;
        float broadPhaseTime;
        float narrowPhaseTime;
        int spatialGridCells;
        int activeColliders;
    };

    CollisionStats GetStatistics() const { return m_stats; }
    void ResetStatistics();

private:
    // Spatial partitioning
    std::unique_ptr<SpatialGrid> m_spatialGrid;
    bool m_useSpatialGrid;

    // Collision optimization
    int m_maxCollisionChecks;
    bool m_collisionCaching;
    float m_collisionDistanceThreshold;
    std::unordered_set<std::string> m_collisionCache;

    // Performance monitoring
    CollisionStats m_stats;
    std::chrono::steady_clock::time_point m_lastUpdateTime;

    // Helper methods
    std::string GetCollisionPairKey(Collision* a, Collision* b);
    void UpdateSpatialGrid();
    std::vector<Collision*> GetPotentialCollisions(const Collision& collision);
    bool ShouldCheckCollisionPair(Collision* a, Collision* b);
    void PerformNarrowPhaseCollision(const std::vector<CollisionPair>& pairs);
};

#endif // OPTIMIZED_COLLISION_SYSTEM_H