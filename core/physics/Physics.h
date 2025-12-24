#ifndef PHYSICS_H
#define PHYSICS_H

#include <memory>
#include <raylib.h>
#include <vector>

class Collision; // Forward declaration moved outside the namespace

namespace CHEngine
{

class Physics
{
public:
    // Raycast down against precise colliders to find ground beneath a point
    static bool RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                            Vector3 &hitPoint, Vector3 &hitNormal);

    // General collision check
    static bool CheckCollision(const ::Collision &collider);
    static bool CheckCollision(const ::Collision &collider, Vector3 &response);

    static void Render();

    // Manual collider management (if needed)
    // static void AddCollider(std::shared_ptr<class Collision> collider);
    // static void ClearColliders();
};

} // namespace CHEngine

#endif // PHYSICS_H
