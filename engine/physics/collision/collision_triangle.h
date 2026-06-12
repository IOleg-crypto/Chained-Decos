#ifndef CH_COLLISION_TRIANGLE_H
#define CH_COLLISION_TRIANGLE_H

#include <glm/glm.hpp>

namespace Chained
{
struct Ray;
struct CollisionTriangle
{
    glm::vec3 v0, v1, v2;
    glm::vec3 min, max;
    glm::vec3 center;
    glm::vec3 normal;
    int meshIndex = -1;

    CollisionTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int index = -1);
    bool IntersectsRay(const Ray& ray, float& t, glm::vec3& normal) const;
};
} // namespace Chained

#endif // CH_COLLISION_TRIANGLE_H
