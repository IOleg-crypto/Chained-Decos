#include "collision_triangle.h"
#include "raymath.h"
#include <cmath>

namespace CHEngine
{
CollisionTriangle::CollisionTriangle(const Vector3& a, const Vector3& b, const Vector3& c, int index)
    : v0(a),
      v1(b),
      v2(c),
      meshIndex(index)
{
    min = Vector3Min(Vector3Min(v0, v1), v2);
    max = Vector3Max(Vector3Max(v0, v1), v2);
    center = Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
    normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v1, v0), Vector3Subtract(v2, v0)));
}

bool CollisionTriangle::IntersectsRay(const Ray& ray, float& t, Vector3& normal) const
{
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 pvec = Vector3CrossProduct(ray.direction, edge2);
    float det = Vector3DotProduct(edge1, pvec);

    if (std::abs(det) < 1e-7f)
    {
        return false;
    }

    float invDet = 1.0f / det;
    Vector3 tvec = Vector3Subtract(ray.position, v0);
    float u = Vector3DotProduct(tvec, pvec) * invDet;

    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    Vector3 qvec = Vector3CrossProduct(tvec, edge1);
    float v = Vector3DotProduct(ray.direction, qvec) * invDet;

    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    float tempT = Vector3DotProduct(edge2, qvec) * invDet;
    if (tempT < 1e-6f)
    {
        return false;
    }

    t = tempT;
    normal = this->normal; // Copy the pre-calculated normal
    if (Vector3DotProduct(normal, ray.direction) > 0)
    {
        normal = Vector3Scale(normal, -1.0f);
    }

    return true;
}
} // namespace CHEngine
