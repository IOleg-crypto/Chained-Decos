#ifndef COLLISIONUTILS_H
#define COLLISIONUTILS_H

#include <raylib.h>
#include <math.h>


inline bool IsFloorResponse(const Vector3& response, const Vector3& velocity, bool isJumping)
{
    float absX = fabsf(response.x);
    float absY = fabsf(response.y);
    float absZ = fabsf(response.z);

    const float FLOOR_DOMINANCE = 1.5f;

    return (
        response.y > 0.05f &&                // > 0.05f замість 0.01f (дрібні значення ігноруються)
        velocity.y <= 0.0f &&
        !isJumping &&
        absY > absX * FLOOR_DOMINANCE &&
        absY > absZ * FLOOR_DOMINANCE
    );
}

inline bool IsWallResponse(const Vector3& response)
{
    float absX = fabsf(response.x);
    float absZ = fabsf(response.z);
    return (absX > absZ ? absX : absZ) > fabsf(response.y);
}


inline Vector3 LimitResponse(const Vector3& response, bool isFallingFast)
{
    Vector3 out = response;

    const float MAX_FLOOR = isFallingFast ? 0.5f : 2.0f;
    const float MAX_WALL  = isFallingFast ? 0.5f : 1.0f;

    if (response.y > 0) out.y = fminf(response.y, MAX_FLOOR);
    else if (response.y < 0) out.y = fmaxf(response.y, -MAX_FLOOR);

    if (fabsf(response.x) > MAX_WALL)
        out.x = response.x > 0 ? MAX_WALL : -MAX_WALL;

    if (fabsf(response.z) > MAX_WALL)
        out.z = response.z > 0 ? MAX_WALL : -MAX_WALL;

    return out;
}

#endif /* COLLISIONUTILS_H */
