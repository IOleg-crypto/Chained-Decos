#ifndef PHYSICS_CPP
#define PHYSICS_CPP
#include "physics.h"
#include "components/physics/collision/core/collision_manager.h"
#endif

using namespace CHEngine;

bool Physics::RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                          Vector3 &hitPoint, Vector3 &hitNormal)
{
    return CollisionManager::RaycastDown(origin, maxDistance, hitDistance, hitPoint, hitNormal);
}

bool Physics::CheckCollision(const Collision &collider)
{
    return CollisionManager::CheckCollision(collider);
}

bool Physics::CheckCollision(const Collision &collider, Vector3 &response)
{
    return CollisionManager::CheckCollision(collider, response);
}

void Physics::Render()
{
    if (!CollisionManager::IsInitialized())
        return; // Or handle error
    CollisionManager::Render();
}
