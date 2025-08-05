//
// Created by I#Oleg
//

#include "CollisionSystem.h"
#include <cmath>

// Методи CollisionBox
CollisionBox CollisionBox::FromCenterAndSize(Vector3 center, Vector3 size) {
    Vector3 halfSize = Vector3Scale(size, 0.5f);
    return CollisionBox(
        Vector3Subtract(center, halfSize),
        Vector3Add(center, halfSize)
    );
}

Vector3 CollisionBox::GetCenter() const {
    return Vector3Scale(Vector3Add(min, max), 0.5f);
}

Vector3 CollisionBox::GetSize() const {
    return Vector3Subtract(max, min);
}

// Методи CollisionSystem
bool CollisionSystem::CheckSphereSphere(const CollisionSphere& a, const CollisionSphere& b) {
    return Vector3Distance(a.center, b.center) <= (a.radius + b.radius);
}

bool CollisionSystem::CheckSphereAABB(const CollisionSphere& sphere, const CollisionBox& box) {
    Vector3 closest = GetClosestPointOnAABB(sphere.center, box);
    return Vector3Distance(sphere.center, closest) <= sphere.radius;
}

bool CollisionSystem::CheckAABBAABB(const CollisionBox& a, const CollisionBox& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

Vector3 CollisionSystem::GetClosestPointOnAABB(const Vector3& point, const CollisionBox& box) {
    return {
        Clamp(point.x, box.min.x, box.max.x),
        Clamp(point.y, box.min.y, box.max.y),
        Clamp(point.z, box.min.z, box.max.z)
    };
}

Vector3 CollisionSystem::GetSphereSphereResponse(const CollisionSphere& a, const CollisionSphere& b) {
    Vector3 dir = Vector3Subtract(a.center, b.center);
    float dist = Vector3Length(dir);
    if (dist > 0) {
        float penetration = (a.radius + b.radius) - dist;
        if (penetration > 0) {
            return Vector3Scale(Vector3Normalize(dir), penetration);
        }
    }
    return {0, 0, 0};
}

Vector3 CollisionSystem::GetSphereAABBResponse(const CollisionSphere& sphere, const CollisionBox& box) {
    Vector3 closest = GetClosestPointOnAABB(sphere.center, box);
    Vector3 dir = Vector3Subtract(sphere.center, closest);
    float dist = Vector3Length(dir);
    if (dist > 0) {
        float penetration = sphere.radius - dist;
        if (penetration > 0) {
            return Vector3Scale(Vector3Normalize(dir), penetration);
        }
    }
    return {0, 0, 0};
}

bool CollisionSystem::SweepSphereAABB(const CollisionSphere &sphere, const Vector3 &velocity,
                                      const CollisionBox &box, float &hitTime, Vector3 &hitNormal)
{
    // Expand the AABB by the sphere's radius
    CollisionBox expandedBox;
    expandedBox.min = Vector3Subtract(box.min, {sphere.radius, sphere.radius, sphere.radius});
    expandedBox.max = Vector3Add(box.max, {sphere.radius, sphere.radius, sphere.radius});

    // Ray-AABB intersection test
    Vector3 invVel = {velocity.x != 0.0f ? 1.0f / velocity.x : INFINITY,
                      velocity.y != 0.0f ? 1.0f / velocity.y : INFINITY,
                      velocity.z != 0.0f ? 1.0f / velocity.z : INFINITY};

    float t1 = (expandedBox.min.x - sphere.center.x) * invVel.x;
    float t2 = (expandedBox.max.x - sphere.center.x) * invVel.x;
    float t3 = (expandedBox.min.y - sphere.center.y) * invVel.y;
    float t4 = (expandedBox.max.y - sphere.center.y) * invVel.y;
    float t5 = (expandedBox.min.z - sphere.center.z) * invVel.z;
    float t6 = (expandedBox.max.z - sphere.center.z) * invVel.z;

    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

    if (tmax < 0 || tmin > tmax || tmin > 1.0f)
    {
        return false; // No intersection within the movement frame
    }

    hitTime = tmin < 0 ? 0 : tmin;

    // Calculate hit normal
    Vector3 hitPoint = Vector3Add(sphere.center, Vector3Scale(velocity, hitTime));
    Vector3 closestPoint = GetClosestPointOnAABB(hitPoint, box);
    hitNormal = Vector3Normalize(Vector3Subtract(hitPoint, closestPoint));

    return true;
}

void CollisionSystem::DrawCollisionSphere(const CollisionSphere& sphere, Color color) {
    DrawSphereWires(sphere.center, sphere.radius, 8, 8, color);
}

void CollisionSystem::DrawCollisionBox(const CollisionBox& box, Color color) {
    Vector3 size = box.GetSize();
    DrawCubeWires(box.GetCenter(), size.x, size.y, size.z, color);
}

// CollisionComponent implementation
void CollisionComponent::UpdateBounds(Vector3 position, float scale)
{
    if (type == SPHERE || type == BOTH)
    {
        sphere.center = position;
        sphere.radius *= scale;
    }

    if (type == AABB || type == BOTH)
    {
        Vector3 size = box.GetSize();
        size = Vector3Scale(size, scale);
        Vector3 halfSize = Vector3Scale(size, 0.5f);
        box.min = Vector3Subtract(position, halfSize);
        box.max = Vector3Add(position, halfSize);
    }
}

void CollisionComponent::UpdateBounds(Vector3 position, Vector3 size)
{
    if (type == SPHERE || type == BOTH)
    {
        sphere.center = position;
        // Use the largest dimension as radius
        sphere.radius = fmaxf(fmaxf(size.x, size.y), size.z) * 0.5f;
    }

    if (type == AABB || type == BOTH)
    {
        Vector3 halfSize = Vector3Scale(size, 0.5f);
        box.min = Vector3Subtract(position, halfSize);
        box.max = Vector3Add(position, halfSize);
    }
}