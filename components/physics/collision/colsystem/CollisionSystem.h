#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <limits>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

#include "components/physics/collision/structures/CollisionStructures.h"

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

struct CollisionResult
{
    bool hit = false;
    Vector3 mtv{}; // Minimum Translation Vector (push out vector)
    Vector3 normal{};
    float depth = 0.0f;
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
    // AABB getters (using raylib BoundingBox)
    Vector3 GetMin() const;
    Vector3 GetMax() const;
    BoundingBox GetBoundingBox() const;
    Vector3 GetCenter() const;
    Vector3 GetSize() const;

    // Update AABB
    void Update(const Vector3 &center, const Vector3 &halfSize);

    // AABB tests (using raylib functions)
    bool IntersectsAABB(const Collision &other) const;

    void BuildFromModel(void *model, const Matrix &transform = MatrixIdentity());

    // Collision type control
    CollisionType GetCollisionType() const;
    void SetCollisionType(CollisionType type);

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
    bool RaycastBVH(const Ray &ray, float maxDistance, RayHit &outHit) const;

    // Intersection with another Collision (broad-phase AABB then BVH narrow-phase)
    bool Intersects(const Collision &other) const;

    // Detailed intersection info (Narrow-phase with MTV)
    CollisionResult CheckCollisionDetailed(const Collision &other) const;

    // Debug visualization
    void DrawDebug(Color color = GREEN, bool drawBVH = false) const;

private:
    // BVH traversal for detailed collision
    CollisionResult CheckBVHOverlapDetailed(const BVHNode *node, const Collision &aabb) const;

    // Helper for triangle-AABB SAT with MTV
    static CollisionResult GetTriangleAABBIntersection(const CollisionTriangle &tri,
                                                       const Vector3 &bmin, const Vector3 &bmax);

    // Internal BVH helpers
    std::unique_ptr<BVHNode> BuildBVHNode(std::vector<CollisionTriangle> &tris, int depth = 0);
    bool RaycastBVHNode(const BVHNode *node, const Ray &ray, float maxDistance,
                        RayHit &outHit) const;
    void DrawDebugBVHNode(const BVHNode *node, int depth, bool leafOnly) const;

public:
    // Compatibility helpers expected by CollisionManager
    bool IsUsingBVH() const;
    void UpdateAABBFromTriangles();

private:
    // AABB using raylib BoundingBox directly
    BoundingBox m_bounds{};

    CollisionType m_collisionType = CollisionType::AABB_ONLY;
    std::vector<CollisionTriangle> m_triangles;

    // BVH root
    std::unique_ptr<BVHNode> m_bvhRoot;

    // Cached flags
    bool m_isBuilt = false;

private:
    // Triangle / AABB helpers
    static void ExpandAABB(Vector3 &minOut, Vector3 &maxOut, const Vector3 &p);

    // Moller-Trumbore ray/triangle
    static bool RayIntersectsTriangle(const Ray &ray, const CollisionTriangle &tri, RayHit &outHit);
};

// CollisionManager is defined in CollisionManager.h

#endif // COLLISIONSYSTEM_H
