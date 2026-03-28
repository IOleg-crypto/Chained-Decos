#include "collision_triangle.h"
#include <cmath>
#include <glm/glm.hpp>

namespace CHEngine
{
CollisionTriangle::CollisionTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, int index)
    : v0(a),
      v1(b),
      v2(c),
      meshIndex(index)
{
    min = glm::min(glm::min(v0, v1), v2);
    max = glm::max(glm::max(v0, v1), v2);
    center = (v0 + v1 + v2) / 3.0f;
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
}

bool CollisionTriangle::IntersectsRay(const Ray& ray, float& t, glm::vec3& normal) const
{
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 pvec = glm::cross(ray.direction, edge2);
    float det = glm::dot(edge1, pvec);

    if (std::abs(det) < 1e-7f) return false;

    float invDet = 1.0f / det;
    glm::vec3 tvec = ray.position - v0;
    float u = glm::dot(tvec, pvec) * invDet;

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(ray.direction, qvec) * invDet;

    if (v < 0.0f || u + v > 1.0f) return false;

    float tempT = glm::dot(edge2, qvec) * invDet;
    if (tempT < 1e-6f) return false;

    t = tempT;
    normal = this->normal;
    if (glm::dot(normal, ray.direction) > 0) normal = -normal;

    return true;
}
} // namespace CHEngine
