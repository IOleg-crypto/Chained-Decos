#ifndef CH_COLLISION_H
#define CH_COLLISION_H

#include "raylib.h"

namespace CHEngine
{
namespace Collision
{
static bool CheckAABB(const Vector3 &minA, const Vector3 &maxA, const Vector3 &minB,
                      const Vector3 &maxB)
{
    return (minA.x <= maxB.x && maxA.x >= minB.x) && (minA.y <= maxB.y && maxA.y >= minB.y) &&
           (minA.z <= maxB.z && maxA.z >= minB.z);
}
} // namespace Collision
} // namespace CHEngine

#endif // CH_COLLISION_H
