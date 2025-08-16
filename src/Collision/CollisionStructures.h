//
// CollisionStructures.h - Common structures for collision system

#ifndef COLLISIONSTRUCTURES_H
#define COLLISIONSTRUCTURES_H

#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

//
// CollisionTriangle - represents a triangle in 3D space for collision detection
//
struct CollisionTriangle
{
    Vector3 v0, v1, v2; // Triangle vertices
    Vector3 normal;     // Triangle normal

    CollisionTriangle() = default;
    CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c);

    // Ray-triangle intersection using Möller-Trumbore algorithm
    bool Intersects(const struct CollisionRay &ray, float &t) const;
    bool Intersects(const Vector3 &origin, const Vector3 &direction, float &t) const;

    // Triangle-triangle intersection
    bool Intersects(const CollisionTriangle &other) const;

    // Triangle-AABB intersection
    bool IntersectsAABB(const Vector3 &boxMin, const Vector3 &boxMax) const;

    // Get triangle properties
    Vector3 GetCenter() const;
    Vector3 GetMin() const;
    Vector3 GetMax() const;
    float GetArea() const;
};

//
// CollisionRay - represents a ray for ray casting
//
struct CollisionRay
{
    Vector3 origin;
    Vector3 direction; // Should be normalized

    CollisionRay() = default;
    CollisionRay(const Vector3 &orig, const Vector3 &dir);
};

//
// CollisionType - determines which collision method to use
//
enum class CollisionType
{
    AABB_ONLY,       // Simple AABB collision (fast, less precise)
    OCTREE_ONLY,     // Octree collision (slower, more precise)
    HYBRID_AUTO,     // Automatically choose based on model complexity
    IMPROVED_AABB,   // Smaller AABB blocks within octree (balanced)
    TRIANGLE_PRECISE // Triangle-to-triangle collision (most precise)
};

//
// CollisionComplexity - helper to determine model complexity
//
struct CollisionComplexity
{
    size_t triangleCount = 0;
    float surfaceArea = 0.0f;
    float boundingVolume = 0.0f;
    bool hasComplexGeometry = false;

    // Thresholds for complexity determination
    static constexpr size_t SIMPLE_TRIANGLE_THRESHOLD = 100;
    static constexpr float SIMPLE_AREA_THRESHOLD = 1000.0f;

    bool IsSimple() const
    {
        return triangleCount <= SIMPLE_TRIANGLE_THRESHOLD && surfaceArea <= SIMPLE_AREA_THRESHOLD &&
               !hasComplexGeometry;
    }

    bool IsComplex() const { return !IsSimple(); }
};

#endif // COLLISIONSTRUCTURES_H