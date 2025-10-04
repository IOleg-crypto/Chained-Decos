//
// CollisionStructures.cpp - Implementation of collision structures
//

#include "CollisionStructures.h"
#include <algorithm>
#include <cmath>

static constexpr float EPS = 1e-6f;

CollisionTriangle::CollisionTriangle(const Vector3 &a, const Vector3 &b, const Vector3 &c)
    : m_v0(a), m_v1(b), m_v2(c)
{
    // Edges
    m_e0 = Vector3Subtract(m_v1, m_v0);
    m_e1 = Vector3Subtract(m_v2, m_v0);

    // Normal
    m_normal = Vector3Normalize(Vector3CrossProduct(m_e0, m_e1));

    // Min/Max
    m_min.x = std::min(std::min(m_v0.x, m_v1.x), m_v2.x);
    m_min.y = std::min(std::min(m_v0.y, m_v1.y), m_v2.y); // FIXED
    m_min.z = std::min(std::min(m_v0.z, m_v1.z), m_v2.z);

    m_max.x = std::max(std::max(m_v0.x, m_v1.x), m_v2.x);
    m_max.y = std::max(std::max(m_v0.y, m_v1.y), m_v2.y);
    m_max.z = std::max(std::max(m_v0.z, m_v1.z), m_v2.z);

    // Center
    m_center = {(m_v0.x + m_v1.x + m_v2.x) / 3.0f, (m_v0.y + m_v1.y + m_v2.y) / 3.0f,
                (m_v0.z + m_v1.z + m_v2.z) / 3.0f};

    // Area
    m_area = 0.5f * Vector3Length(Vector3CrossProduct(m_e0, m_e1));

    // Precompute dot products for barycentric coordinates
    m_dot00 = Vector3DotProduct(m_e1, m_e1);
    m_dot01 = Vector3DotProduct(m_e1, m_e0);
    m_dot11 = Vector3DotProduct(m_e0, m_e0);
}

bool CollisionTriangle::Intersects(const CollisionRay &ray, float &t) const
{
    // Möller-Trumbore ray-triangle intersection algorithm
    Vector3 edge1 = Vector3Subtract(m_v1, m_v0);
    Vector3 edge2 = Vector3Subtract(m_v2, m_v0); // FIXED
    Vector3 h = Vector3CrossProduct(ray.GetDirection(), edge2);
    float a = Vector3DotProduct(edge1, h);

    if (a > -EPS && a < EPS)
        return false; // Ray is parallel to triangle

    float f = 1.0f / a;
    Vector3 s = Vector3Subtract(ray.GetOrigin(), m_v0);
    float u = f * Vector3DotProduct(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = Vector3CrossProduct(s, edge1);
    float v = f * Vector3DotProduct(ray.GetDirection(), q);

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
    float d1 = Vector3DotProduct(m_normal, Vector3Subtract(other.m_v0, m_v0));
    float d2 = Vector3DotProduct(m_normal, Vector3Subtract(other.m_v1, m_v0));
    float d3 = Vector3DotProduct(m_normal, Vector3Subtract(other.m_v2, m_v0));

    // If all points of other triangle are on same side of this triangle's plane
    if ((d1 > EPS && d2 > EPS && d3 > EPS) || (d1 < -EPS && d2 < -EPS && d3 < -EPS))
        return false;

    // Check the other direction
    d1 = Vector3DotProduct(other.m_normal, Vector3Subtract(m_v0, other.m_v0));
    d2 = Vector3DotProduct(other.m_normal, Vector3Subtract(m_v1, other.m_v0));
    d3 = Vector3DotProduct(other.m_normal, Vector3Subtract(m_v2, other.m_v0));

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
    Vector3 tv0 = Vector3Subtract(m_v0, boxCenter);
    Vector3 tv1 = Vector3Subtract(m_v1, boxCenter); // FIXED
    Vector3 tv2 = Vector3Subtract(m_v2, boxCenter); // FIXED

    // Test triangle normal as separating axis
    float r = boxExtents.x * fabsf(m_normal.x) + boxExtents.y * fabsf(m_normal.y) +
              boxExtents.z * fabsf(m_normal.z);
    float p0 = Vector3DotProduct(tv0, m_normal);
    float p1 = Vector3DotProduct(tv1, m_normal);
    float p2 = Vector3DotProduct(tv2, m_normal);
    float minP = fminf(fminf(p0, p1), p2);
    float maxP = fmaxf(fmaxf(p0, p1), p2);

    if (minP > r || maxP < -r)
        return false;

    // If we get here, there's likely an intersection
    return true;
}

bool CollisionComplexity::IsSimple() const
{
    return m_triangleCount <= SIMPLE_TRIANGLE_THRESHOLD && m_surfaceArea <= SIMPLE_AREA_THRESHOLD &&
           !m_hasComplexGeometry;
}

// ================== CollisionRay Implementation ==================

CollisionRay::CollisionRay(const Vector3 &orig, const Vector3 &dir)
    : m_origin(orig), m_direction(Vector3Normalize(dir))
{
}
bool CollisionComplexity::IsComplex() const { return !IsSimple(); }
Vector3 CollisionTriangle::GetCenter() const { return m_center; }
Vector3 CollisionTriangle::GetMin() const { return m_min; }
Vector3 CollisionTriangle::GetMax() const { return m_max; }
float CollisionTriangle::GetArea() const { return m_area; }
Vector3 CollisionTriangle::GetNormal() const { return m_normal; }
const Vector3 &CollisionTriangle::V0() const { return m_v0; }
const Vector3 &CollisionTriangle::V1() const { return m_v1; }
const Vector3 &CollisionTriangle::V2() const { return m_v2; }

const Vector3 &CollisionRay::GetOrigin() const { return m_origin; }
const Vector3 &CollisionRay::GetDirection() const { return m_direction; }
