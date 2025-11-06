#ifndef COLLISIONSTRUCTURES_H
#define COLLISIONSTRUCTURES_H

#include <cstdint>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <vector>

//
// CollisionRay - represents a ray for ray casting
//
class CollisionRay
{
public:
    CollisionRay() = default;
    CollisionRay(const Vector3 &orig, const Vector3 &dir);

    const Vector3 &GetOrigin() const;
    const Vector3 &GetDirection() const;

private:
    Vector3 m_origin{};
    Vector3 m_direction{};
};

//
// CollisionTriangle - represents a triangle in 3D space for collision detection
//
class CollisionTriangle
{
public:
    CollisionTriangle() = default;
    CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c);

    // Ray-triangle intersection using Möller-Trumbore algorithm
    bool Intersects(const CollisionRay &ray, float &t) const;
    bool Intersects(const Vector3 &origin, const Vector3 &direction, float &t) const;

    // Get triangle properties
    Vector3 GetCenter() const;
    Vector3 GetMin() const;
    Vector3 GetMax() const;
    float GetArea() const;
    Vector3 GetNormal() const;

    const Vector3 &V0() const;
    const Vector3 &V1() const;
    const Vector3 &V2() const;

private:
    Vector3 m_v0{}, m_v1{}, m_v2{}; // Triangle vertices
    Vector3 m_min{}, m_max{};       // Bounding Box
    Vector3 m_normal{};             // Triangle normal
    Vector3 m_center{};             // Cached triangle center
    float m_area{};                 // Cached area

    // Cached edge vectors for normal and area calculations
    Vector3 m_e0{}, m_e1{};
};

//
// CollisionType - determines which collision method to use
//
enum class CollisionType : uint8_t
{
    AABB_ONLY,       // Simple AABB collision (fast, less precise)
    BVH_ONLY,        // BVH-based collision (precise, scalable)
    HYBRID_AUTO,     // Automatically choose based on model complexity
    TRIANGLE_PRECISE // Brute force (triangle-to-triangle)
};

#endif // COLLISIONSTRUCTURES_H