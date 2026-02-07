#ifndef CH_COLLISION_H
#define CH_COLLISION_H

#include "engine/core/base.h"
#include "raylib.h"
#include "raymath.h"
#include "engine/physics/collision/collision_triangle.h"

namespace CHEngine
{
    /**
     * @brief Static utility class for collision detection math.
     * Follows the Hazel engine architecture pattern of static functionality.
     */
    class Collision
    {
    public:
        /**
         * @brief Finds the closest point on a triangle to a given target point.
         */
        static Vector3 GetClosestPointOnTriangle(const Vector3& targetPoint, 
                                                 const CollisionTriangle& triangle);

        /**
         * @brief Tests intersection between a sphere and a triangle.
         * Used for smooth character movement.
         */
        static bool IntersectSpherevsTriangle(const Vector3& sphereCenter, 
                                              float sphereRadius, 
                                              const CollisionTriangle& triangle, 
                                              Vector3& outMinimumTranslationVector);
    };
}

#endif // CH_COLLISION_H
