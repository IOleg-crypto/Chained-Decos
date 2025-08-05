//
// Created by I#Oleg
//

#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include <raylib.h>
#include <raymath.h>
#include <vector>

// Сфера для колізії
struct CollisionSphere
{
    Vector3 center;
    float radius;
    CollisionSphere(Vector3 c = {0,0,0}, float r = 1.0f) : center(c), radius(r) {}
};

// AABB для колізії
struct CollisionBox
{
    Vector3 min;
    Vector3 max;
    CollisionBox(Vector3 minP = {0,0,0}, Vector3 maxP = {0,0,0}) : min(minP), max(maxP) {}

    static CollisionBox FromCenterAndSize(Vector3 center, Vector3 size);
    Vector3 GetCenter() const;
    Vector3 GetSize() const;
};

// Головна система колізій
class CollisionSystem
{
public:
    static bool CheckSphereSphere(const CollisionSphere& a, const CollisionSphere& b);

    static bool CheckSphereAABB(const CollisionSphere& sphere, const CollisionBox& box);

    static bool CheckAABBAABB(const CollisionBox& a, const CollisionBox& b);

    static Vector3 GetClosestPointOnAABB(const Vector3& point, const CollisionBox& box);

    static Vector3 GetSphereSphereResponse(const CollisionSphere& a, const CollisionSphere& b);

    static Vector3 GetSphereAABBResponse(const CollisionSphere& sphere, const CollisionBox& box);

    static void DrawCollisionSphere(const CollisionSphere& sphere, Color color);

    static void DrawCollisionBox(const CollisionBox& box, Color color);

    bool CollisionSystem::SweepSphereAABB(const CollisionSphere &sphere, const Vector3 &velocity,
        const CollisionBox &box, float &hitTime, Vector3 &hitNormal);
};

// Collision component that can be attached to game objects
struct CollisionComponent
{
    enum CollisionType
    {
        SPHERE,
        AABB,
        BOTH
    };

    CollisionType type;
    CollisionSphere sphere;
    CollisionBox box;
    bool isStatic; // Static objects don't move when collided with
    bool isActive; // Can be disabled temporarily

    CollisionComponent()
    {
        type = SPHERE;
        isStatic = false;
        isActive = true;
    }

    // Update collision bounds based on position and scale
    void UpdateBounds(Vector3 position, float scale);
    void UpdateBounds(Vector3 position, Vector3 size);
};

#endif // COLLISIONSYSTEM_H