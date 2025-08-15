//
// Created by I#Oleg
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <cfloat>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

// Forward declarations
struct CollisionTriangle;
struct BVHNode;
struct CollisionRay;

// Triangle structure for precise collision detection
struct CollisionTriangle
{
    Vector3 v0, v1, v2; // Triangle vertices
    Vector3 normal;     // Triangle normal

    CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c);
    bool Intersects(const CollisionRay &ray, float &t) const;
    Vector3 GetCenter() const;
    Vector3 GetMin() const;
    Vector3 GetMax() const;
};

// Ray structure for collision queries (renamed to avoid conflict with raylib's Ray)
struct CollisionRay
{
    Vector3 origin;
    Vector3 direction;

    CollisionRay(const Vector3 &orig, const Vector3 &dir);
};

// BVH Node structure
struct BVHNode
{
    Vector3 minBounds, maxBounds;             // Bounding box
    std::unique_ptr<BVHNode> left, right;     // Child nodes
    std::vector<CollisionTriangle> triangles; // Leaf triangles
    bool isLeaf;

    BVHNode();
    bool Intersects(const Vector3 &min, const Vector3 &max) const;
    bool IntersectsRay(const CollisionRay &ray, float &t) const;
};

//
// Collision
// Handles collision detection with both AABB and BVH support.
// Provides methods for fast broad-phase and precise narrow-phase collision detection.
//
class Collision
{
public:
    Collision() = default;

    // Initialize collision box by center position and size (half-extents)
    Collision(const Vector3 &center, const Vector3 &size);

    // Copy constructor and assignment operator (needed due to std::unique_ptr)
    Collision(const Collision &other);
    Collision &operator=(const Collision &other);

    // Move constructor and assignment operator
    Collision(Collision &&other) noexcept = default;
    Collision &operator=(Collision &&other) noexcept = default;

    ~Collision() = default;

    // -------------------- Getters --------------------

    // Get minimum corner of bounding box
    Vector3 GetMin() const;

    // Get maximum corner of bounding box
    Vector3 GetMax() const;

    // -------------------- Update --------------------

    // Update bounding box position and size
    void Update(const Vector3 &center, const Vector3 &size);

    // -------------------- Collision Checks --------------------

    // Check if this collision box intersects with another
    bool Intersects(const Collision &other) const;

    // Check if this collision box contains a point
    bool Contains(const Vector3 &point) const;

    // -------------------- Model Collision --------------------
    void CalculateFromModel(Model *model);
    void CalculateFromModel(Model *model, const Matrix &transform);

    Vector3 GetCenter() const;
    Vector3 GetSize() const;

    // -------------------- BVH Methods --------------------

    // Build BVH from model for precise collision detection
    void BuildBVH(Model *model);
    void BuildBVH(Model *model, const Matrix &transform);

    // Check collision using BVH (more precise than AABB)
    bool IntersectsBVH(const Collision &other) const;

    // Ray casting with BVH
    bool RaycastBVH(const CollisionRay &ray, float &distance, Vector3 &hitPoint,
                    Vector3 &hitNormal) const;

    // Point-in-mesh test using BVH
    bool ContainsBVH(const Vector3 &point) const;

    // Enable/disable BVH usage
    void SetUseBVH(bool useBVH) { m_useBVH = useBVH; }
    bool IsUsingBVH() const { return m_useBVH && m_bvhRoot != nullptr; }

    // Get triangle count in BVH
    size_t GetTriangleCount() const;

private:
    Vector3 m_min{}; // Minimum corner of AABB
    Vector3 m_max{}; // Maximum corner of AABB
    Mesh m_mesh{};   // Optional mesh for rendering (not used in this example)

    // BVH data
    std::unique_ptr<BVHNode> m_bvhRoot;
    std::vector<CollisionTriangle> m_triangles;
    bool m_useBVH = false;

    // BVH helper methods
    std::unique_ptr<BVHNode> BuildBVHRecursive(std::vector<CollisionTriangle> &triangles,
                                               int depth = 0);
    void ExtractTrianglesFromModel(Model *model, const Matrix *transform = nullptr);
    bool IntersectsBVHRecursive(const BVHNode *node, const Vector3 &otherMin,
                                const Vector3 &otherMax) const;

    // Precise triangle-AABB intersection test
    bool TriangleIntersectsAABB(const CollisionTriangle &triangle, const Vector3 &boxMin,
                                const Vector3 &boxMax) const;
};

#endif // COLLISIONSYSTEM_H
