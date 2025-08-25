//
// CollisionStructures.cpp - Implementation of collision structures
//

#include "CollisionStructures.h"
#include <algorithm>
#include <cmath>

static constexpr float EPS = 1e-6f;

CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : v0(a), v1(b), v2(c)
{
    // Edges
    e0 = Vector3Subtract(v1, v0);
    e1 = Vector3Subtract(v2, v0);

    // Normal
    normal = Vector3Normalize(Vector3CrossProduct(e0, e1));

    // Min/Max
    min.x = std::min(std::min(v0.x, v1.x), v2.x);
    min.y = std::min(std::min(v0.y, v1.y), v2.y);
    min.z = std::min(std::min(v0.z, v1.z), v2.z);

    max.x = std::max(std::max(v0.x, v1.x), v2.x);
    max.y = std::max(std::max(v0.y, v1.y), v2.y);
    max.z = std::max(std::max(v0.z, v1.z), v2.z);

    // Center
    center = {(v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f,
              (v0.z + v1.z + v2.z) / 3.0f};

    // Area
    area = 0.5f * Vector3Length(Vector3CrossProduct(e0, e1));

    // Precompute dot products for barycentric coordinates
    dot00 = Vector3DotProduct(e1, e1);
    dot01 = Vector3DotProduct(e1, e0);
    dot11 = Vector3DotProduct(e0, e0);
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
    const CollisionRay ray(origin, direction);
    return Intersects(ray, t);
}

bool CollisionTriangle::Intersects(const CollisionTriangle &other) const
{
    // Triangle-triangle intersection using separating axis theorem
    // This is a simplified version - for production use, consider more robust algorithms

    // Check if triangles are coplanar
    float d1 = Vector3DotProduct(normal, Vector3Subtract(other.v0, v0));
    float d2 = Vector3DotProduct(normal, Vector3Subtract(other.v1, v0));
    float d3 = Vector3DotProduct(normal, Vector3Subtract(other.v2, v0));

    // If all points of other triangle are on same side of this triangle's plane
    if ((d1 > EPS && d2 > EPS && d3 > EPS) || (d1 < -EPS && d2 < -EPS && d3 < -EPS))
        return false;

    // Check the other direction
    d1 = Vector3DotProduct(other.normal, Vector3Subtract(v0, other.v0));
    d2 = Vector3DotProduct(other.normal, Vector3Subtract(v1, other.v0));
    d3 = Vector3DotProduct(other.normal, Vector3Subtract(v2, other.v0));

    if ((d1 > EPS && d2 > EPS && d3 > EPS) || (d1 < -EPS && d2 < -EPS && d3 < -EPS))
        return false;

    // If we get here, triangles potentially intersect
    // For simplicity, we'll use AABB intersection as approximation
    Vector3 thisMin = GetMin();
    Vector3 thisMax = GetMax();
    Vector3 otherMin = other.GetMin();
    Vector3 otherMax = other.GetMax();

    return (thisMin.x <= otherMax.x && thisMax.x >= otherMin.x) &&
           (thisMin.y <= otherMax.y && thisMax.y >= otherMin.y) &&
           (thisMin.z <= otherMax.z && thisMax.z >= otherMin.z);
}

bool CollisionTriangle::IntersectsAABB(const Vector3 &boxMin, const Vector3 &boxMax) const
{
    // Triangle-AABB intersection test
    // First check if triangle's AABB intersects with box
    Vector3 triMin = GetMin();
    Vector3 triMax = GetMax();

    if (triMax.x < boxMin.x || triMin.x > boxMax.x || triMax.y < boxMin.y || triMin.y > boxMax.y ||
        triMax.z < boxMin.z || triMin.z > boxMax.z)
        return false;

    // More precise test: check if triangle intersects box
    Vector3 boxCenter = Vector3Scale(Vector3Add(boxMin, boxMax), 0.5f);
    Vector3 boxExtents = Vector3Scale(Vector3Subtract(boxMax, boxMin), 0.5f);

    // Translate triangle to box center
    Vector3 tv0 = Vector3Subtract(v0, boxCenter);
    Vector3 tv1 = Vector3Subtract(v1, boxCenter);
    Vector3 tv2 = Vector3Subtract(v2, boxCenter);

    // Test triangle normal as separating axis
    float r = boxExtents.x * fabsf(normal.x) + boxExtents.y * fabsf(normal.y) +
              boxExtents.z * fabsf(normal.z);
    float p0 = Vector3DotProduct(tv0, normal);
    float p1 = Vector3DotProduct(tv1, normal);
    float p2 = Vector3DotProduct(tv2, normal);
    float minP = fminf(fminf(p0, p1), p2);
    float maxP = fmaxf(fmaxf(p0, p1), p2);

    if (minP > r || maxP < -r)
        return false;

    // If we get here, there's likely an intersection
    return true;
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