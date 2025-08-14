//
// CollisionStructures.cpp - Implementation of collision structures
//

#include "CollisionStructures.h"
#include <cmath>

static constexpr float EPS = 1e-6f;

// ================== CollisionTriangle Implementation ==================

CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : v0(a), v1(b), v2(c)
{
    // Calculate normal using cross product
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
}

bool CollisionTriangle::Intersects(const CollisionRay &ray, float &t) const
{
    // Möller-Trumbore ray-triangle intersection algorithm
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 h = Vector3CrossProduct(ray.direction, edge2);
    float a = Vector3DotProduct(edge1, h);

    if (a > -EPS && a < EPS)
        return false; // Ray is parallel to triangle

    float f = 1.0f / a;
    Vector3 s = Vector3Subtract(ray.origin, v0);
    float u = f * Vector3DotProduct(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = Vector3CrossProduct(s, edge1);
    float v = f * Vector3DotProduct(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * Vector3DotProduct(edge2, q);
    return t > EPS; // Ray intersection
}

bool CollisionTriangle::Intersects(const Vector3 &origin, const Vector3 &direction, float &t) const
{
    CollisionRay ray(origin, direction);
    return Intersects(ray, t);
}

Vector3 CollisionTriangle::GetCenter() const
{
    return Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
}

Vector3 CollisionTriangle::GetMin() const
{
    return {fminf(fminf(v0.x, v1.x), v2.x), fminf(fminf(v0.y, v1.y), v2.y),
            fminf(fminf(v0.z, v1.z), v2.z)};
}

Vector3 CollisionTriangle::GetMax() const
{
    return {fmaxf(fmaxf(v0.x, v1.x), v2.x), fmaxf(fmaxf(v0.y, v1.y), v2.y),
            fmaxf(fmaxf(v0.z, v1.z), v2.z)};
}

float CollisionTriangle::GetArea() const
{
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 cross = Vector3CrossProduct(edge1, edge2);
    return Vector3Length(cross) * 0.5f;
}

// ================== CollisionRay Implementation ==================

CollisionRay::CollisionRay(const Vector3 &orig, const Vector3 &dir)
    : origin(orig), direction(Vector3Normalize(dir))
{
}