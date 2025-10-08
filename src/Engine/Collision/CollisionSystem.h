#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>

#include "CollisionStructures.h"

struct BVHNode
{
    Vector3 min;
    Vector3 max;
    std::vector<CollisionTriangle> triangles;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;

    bool IsLeaf() const;
};

struct RayHit
{
    bool hit = false;
    float distance = std::numeric_limits<float>::infinity();
    Vector3 position{};
    Vector3 normal{};
};

// ------------------ Collision class ------------------
class Collision
{
public:
    Collision();
    Collision(const Vector3 &center, const Vector3 &halfSize);
    ~Collision();
    Collision(const Collision &other);
    Collision &operator=(const Collision &other);
    Collision(Collision &&other) noexcept;
    Collision &operator=(Collision &&other) noexcept;

public:
    // AABB getters
    Vector3 GetMin() const;
    Vector3 GetMax() const;
    Vector3 GetCenter() const;
    Vector3 GetSize() const;

    // Update AABB
    void Update(const Vector3 &center, const Vector3 &halfSize);

    // AABB tests
    bool IntersectsAABB(const Collision &other) const;
    bool ContainsPointAABB(const Vector3 &point) const;

  
    void BuildFromModel(void *model, const Matrix &transform = MatrixIdentity());
    void BuildFromModelWithType(void *model, CollisionType type,
                                const Matrix &transform = MatrixIdentity());
    void CalculateFromModel(void *model, const Matrix &transform = MatrixIdentity());

    // Parallel version for better performance with complex models
    void BuildFromModelParallel(void *model, const Matrix &transform = MatrixIdentity());

    // Optimized collision creation with caching
    static std::shared_ptr<Collision> CreateFromModelCached(void *model, const Matrix &transform = MatrixIdentity());
    static void ClearCollisionCache();

    // Collision type control
    CollisionType GetCollisionType() const;
    void SetCollisionType(CollisionType type);

    const CollisionComplexity &GetComplexity() const;

    const CollisionTriangle &GetTriangle(size_t idx) const;
    const std::vector<CollisionTriangle> &GetTriangles() const;
    void AddTriangle(const CollisionTriangle &triangle);
    void AddTriangles(const std::vector<CollisionTriangle> &triangles);

    // BVH methods
    void BuildBVHFromTriangles();
    size_t GetTriangleCount() const;
    bool HasTriangleData() const;

    // Initialize BVH (compat wrapper for manager)
    void InitializeBVH();

    // Raycast using BVH (returns true if hit within maxDistance)
    bool RaycastBVH(const Vector3 &origin, const Vector3 &dir, float maxDistance,
                    RayHit &outHit) const;

    // Point-in-mesh using BVH (raycast trick)
    bool ContainsPointBVH(const Vector3 &point) const;

    // Intersection with another Collision (broad-phase AABB then BVH narrow-phase)
    bool Intersects(const Collision &other) const;

    // Compatibility helpers expected by CollisionManager (legacy Octree paths)
    bool IntersectsBVH(const Collision &other) const;
    bool IsUsingBVH() const;
    bool IsUsingOctree() const;
    bool RaycastOctree(const Vector3 &origin, const Vector3 &dir, float maxDistance,
                       float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const;

    // Debug / stats
    struct PerformanceStats
    {
        float lastCheckTime = 0.0f;
        size_t checksPerformed = 0;
        CollisionType typeUsed = CollisionType::AABB_ONLY;
    };
    const PerformanceStats &GetPerformanceStats() const;

    bool CheckCollisionWithBVH(const Collision& other, Vector3& outResponse) const;

    void UpdateAABBFromTriangles();

    // Public accessor for cache size
    static size_t GetCollisionCacheSize() { return collisionCache.size(); }

    // Public methods to reset collision state
    void ResetCollisionState()
    {
        m_triangles.clear();
        m_bvhRoot.reset();
        m_isBuilt = false;
    }

    // Public access to collision cache for pool management
    static std::unordered_map<size_t, std::weak_ptr<Collision>>& GetCollisionCache()
    {
        return collisionCache;
    }

private:
    // AABB
    Vector3 m_min{};
    Vector3 m_max{};

    CollisionType m_collisionType = CollisionType::HYBRID_AUTO;
    CollisionComplexity m_complexity;
    std::vector<CollisionTriangle> m_triangles;

    // BVH root
    std::unique_ptr<BVHNode> m_bvhRoot;

    // Cached flags
    bool m_isBuilt = false;

    // Perf stats
    mutable PerformanceStats m_stats;

    // Collision cache for performance optimization
    static std::unordered_map<size_t, std::weak_ptr<Collision>> collisionCache;

private:
    // Helpers
    std::unique_ptr<BVHNode> BuildBVHNode(std::vector<CollisionTriangle> &tris, int depth = 0);
    bool RaycastBVHNode(const BVHNode *node, const Vector3 &origin, const Vector3 &dir,
                        float maxDistance, RayHit &outHit) const;

    // Triangle / AABB helpers
    static void ExpandAABB(Vector3 &minOut, Vector3 &maxOut, const Vector3 &p);
    static void TriangleBounds(const CollisionTriangle &t, Vector3 &outMin, Vector3 &outMax);
    static bool AABBIntersectRay(const Vector3 &min, const Vector3 &max, const Vector3 &origin,
                                 const Vector3 &dir, float maxDistance);

    // Moller-Trumbore ray/triangle
    static bool RayIntersectsTriangle(const Vector3 &orig, const Vector3 &dir,
                                      const CollisionTriangle &tri, RayHit &outHit);
};

// Collision object pool for efficient memory management
class CollisionPool
{
public:
    static CollisionPool& GetInstance();

    // Pool management
    std::shared_ptr<Collision> AcquireCollision();
    void ReleaseCollision(std::shared_ptr<Collision> collision);
    void ClearPool();

    // Statistics
    size_t GetPoolSize() const { return m_collisionPool.size(); }
    size_t GetActiveCollisions() const { return m_activeCollisions.size(); }
    size_t GetCacheSize() const { return Collision::GetCollisionCacheSize(); }

    // Settings
    void SetMaxPoolSize(size_t maxSize) { m_maxPoolSize = maxSize; }
    void SetMaxCacheSize(size_t maxSize) { m_maxCacheSize = maxSize; }

    // Cleanup
    void CleanupUnusedCollisions();
    void CleanupExpiredCache();

private:
    CollisionPool();
    ~CollisionPool();

    std::stack<std::shared_ptr<Collision>> m_collisionPool;
    std::unordered_set<std::shared_ptr<Collision>> m_activeCollisions;
    size_t m_maxPoolSize = 100;
    size_t m_maxCacheSize = 200;
    mutable std::mutex m_poolMutex;

    // Prevent copying
    CollisionPool(const CollisionPool&) = delete;
    CollisionPool& operator=(const CollisionPool&) = delete;
};

// CollisionManager is defined in CollisionManager.h

#endif // COLLISIONSYSTEM_H
