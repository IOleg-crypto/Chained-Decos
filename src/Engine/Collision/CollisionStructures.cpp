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

    // Normal (with safety check)
    Vector3 normalCross = Vector3CrossProduct(m_e0, m_e1);
    float normalLength  = Vector3Length(normalCross);

    if (std::isfinite(normalLength) && normalLength > 1e-12f)
    {
        m_normal = Vector3Scale(normalCross, 1.0f / normalLength);
    }
    else
    {
        // Fallback normal for degenerate triangles
        m_normal = {0.0f, 1.0f, 0.0f};
    }

    // Min/Max
    m_min.x = std::min(std::min(m_v0.x, m_v1.x), m_v2.x);
    m_min.y = std::min(std::min(m_v0.y, m_v1.y), m_v2.y); // FIXED
    m_min.z = std::min(std::min(m_v0.z, m_v1.z), m_v2.z);

    m_max.x = std::max(std::max(m_v0.x, m_v1.x), m_v2.x);
    m_max.y = std::max(std::max(m_v0.y, m_v1.y), m_v2.y);
    m_max.z = std::max(std::max(m_v0.z, m_v1.z), m_v2.z);

    // Center (with safety checks)
    float centerX = (m_v0.x + m_v1.x + m_v2.x) / 3.0f;
    float centerY = (m_v0.y + m_v1.y + m_v2.y) / 3.0f;
    float centerZ = (m_v0.z + m_v1.z + m_v2.z) / 3.0f;

    // Check for finite values
    if (!std::isfinite(centerX) || !std::isfinite(centerY) || !std::isfinite(centerZ))
    {
        // Fallback to first vertex if centroid calculation fails
        m_center = m_v0;
    }
    else
    {
        m_center = {centerX, centerY, centerZ};
    }

    // Area (with safety check)
    Vector3 crossProduct = Vector3CrossProduct(m_e0, m_e1);
    float crossLength    = Vector3Length(crossProduct);

    if (std::isfinite(crossLength))
    {
        m_area = 0.5f * crossLength;
    }
    else
    {
        m_area = 0.0f; // Degenerate triangle
    }
}

bool CollisionTriangle::Intersects(const CollisionRay &ray, float &t) const
{
    // Möller-Trumbore ray-triangle intersection algorithm with enhanced safety checks
    Vector3 edge1 = Vector3Subtract(m_v1, m_v0);
    Vector3 edge2 = Vector3Subtract(m_v2, m_v0);

    // Check for degenerate triangles
    if (Vector3LengthSqr(edge1) < 1e-12f || Vector3LengthSqr(edge2) < 1e-12f)
        return false; // Degenerate triangle

    Vector3 h = Vector3CrossProduct(ray.GetDirection(), edge2);
    float a   = Vector3DotProduct(edge1, h);

    // Enhanced check for parallel rays with better epsilon handling
    const float EPS_PARALLEL = 1e-8f;
    if (fabsf(a) < EPS_PARALLEL)
        return false; // Ray is parallel to triangle

    float f = 1.0f / a;

    // Check for invalid division result
    if (!std::isfinite(f))
        return false;

    Vector3 s = Vector3Subtract(ray.GetOrigin(), m_v0);
    float u   = f * Vector3DotProduct(s, h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = Vector3CrossProduct(s, edge1);
    float v   = f * Vector3DotProduct(ray.GetDirection(), q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * Vector3DotProduct(edge2, q);

    // Check for valid intersection distance
    if (!std::isfinite(t) || t <= EPS)
        return false;

    return t > EPS; // Ray intersection
}

bool CollisionTriangle::Intersects(const Vector3 &origin, const Vector3 &direction, float &t) const
{
    // Validate direction vector before creating ray
    float dirLengthSqr = Vector3LengthSqr(direction);
    if (dirLengthSqr < 1e-12f)
        return false; // Invalid direction vector

    const CollisionRay ray(origin, direction);
    return Intersects(ray, t);
}

CollisionRay::CollisionRay(const Vector3 &orig, const Vector3 &dir) : m_origin(orig), m_direction(dir) {}

const Vector3 &CollisionRay::GetOrigin() const { return m_origin; }
const Vector3 &CollisionRay::GetDirection() const { return m_direction; }
Vector3 CollisionTriangle::GetMin() const { return m_min; }
Vector3 CollisionTriangle::GetMax() const { return m_max; }
float CollisionTriangle::GetArea() const { return m_area; }
Vector3 CollisionTriangle::GetNormal() const { return m_normal; }
const Vector3 &CollisionTriangle::V0() const { return m_v0; }
const Vector3 &CollisionTriangle::V1() const { return m_v1; }
const Vector3 &CollisionTriangle::V2() const { return m_v2; }
