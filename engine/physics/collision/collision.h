#ifndef CH_COLLISION_H
#define CH_COLLISION_H

#include <glm/glm.hpp>

namespace CHEngine
{
namespace Collision
{
static bool CheckAABB(const glm::vec3& minA, const glm::vec3& maxA, const glm::vec3& minB, const glm::vec3& maxB)
{
    return (minA.x <= maxB.x && maxA.x >= minB.x) && (minA.y <= maxB.y && maxA.y >= minB.y) &&
           (minA.z <= maxB.z && maxA.z >= minB.z);
}
} // namespace Collision
} // namespace CHEngine

#endif // CH_COLLISION_H
