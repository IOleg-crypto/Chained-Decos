#ifndef ICOLLISION_MANAGER_H
#define ICOLLISION_MANAGER_H

#include "components/physics/collision/system/collisionSystem.h"
#include <memory>
#include <raylib.h>
#include <scene/ecs/Entity.h>
#include <vector>

class ICollisionManager
{
public:
    virtual ~ICollisionManager() = default;

    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual const std::vector<std::shared_ptr<Collision>> &GetColliders() const = 0;
    virtual bool CheckCollision(const Collision &playerCollision) const = 0;
    virtual bool CheckCollision(const Collision &playerCollision, Vector3 &response) const = 0;
    virtual void AddCollider(std::shared_ptr<Collision> collider) = 0;
    virtual void ClearColliders() = 0;

    // Raycast down against precise colliders to find ground beneath a point
    virtual bool RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                             Vector3 &hitPoint, Vector3 &hitNormal) const = 0;

    // Dynamic Entity Management (ECS Integration)
    virtual void AddEntityCollider(ECS::EntityID entity,
                                   const std::shared_ptr<Collision> &collider) = 0;
    virtual void RemoveEntityCollider(ECS::EntityID entity) = 0;
    virtual void UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position) = 0;
    virtual std::shared_ptr<Collision> GetEntityCollider(ECS::EntityID entity) const = 0;
    virtual bool CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                      std::vector<ECS::EntityID> &outCollidedEntities) const = 0;
};

#endif // ICOLLISION_MANAGER_H
