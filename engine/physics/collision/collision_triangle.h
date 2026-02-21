#ifndef CH_COLLISION_TRIANGLE_H
#define CH_COLLISION_TRIANGLE_H

#include "raylib.h"

namespace CHEngine
{
struct CollisionTriangle
{
    Vector3 v0, v1, v2;
    Vector3 min, max;
    Vector3 center;
    int meshIndex = -1;

    CollisionTriangle(const Vector3& a, const Vector3& b, const Vector3& c, int index = -1);
    bool IntersectsRay(const Ray& ray, float& t, Vector3& normal) const;
};
} // namespace CHEngine

#endif // CH_COLLISION_TRIANGLE_H
