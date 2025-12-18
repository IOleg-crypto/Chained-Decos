#include "CollisionSystem.h"
#include "core/ecs/Components/PhysicsData.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/ECSRegistry.h"
#include <raylib.h>
#include <raymath.h>

namespace CollisionSystem
{

static void OnCollision(entt::entity a, entt::entity b, CollisionComponent &collA,
                        CollisionComponent &collB)
{
    // Set collision state
    collA.hasCollision = true;
    collA.collidedWith = b;

    collB.hasCollision = true;
    collB.collidedWith = a;

    // If this is a trigger - do not block movement
    if (collA.isTrigger || collB.isTrigger)
    {
        return;
    }

    // Collision response can be added here
    // For example, push entities away from each other
}

void Update()
{
    auto view = REGISTRY.view<TransformComponent, CollisionComponent>();

    // Broad phase - fast check
    // Narrow phase - precise check

    for (auto &&[entityA, transformA, collisionA] : view.each())
    {
        // Reset collision state
        collisionA.hasCollision = false;
        collisionA.collidedWith = entt::null;

        for (auto [entityB, transformB, collisionB] : view.each())
        {
            if (entityA == entityB)
                continue;

            // Check collision mask
            if (!(collisionA.collisionMask & (1 << collisionB.collisionLayer)))
            {
                continue;
            }

            // Update bounds
            BoundingBox boundsA = collisionA.bounds;
            boundsA.min = Vector3Add(boundsA.min, transformA.position);
            boundsA.max = Vector3Add(boundsA.max, transformA.position);

            BoundingBox boundsB = collisionB.bounds;
            boundsB.min = Vector3Add(boundsB.min, transformB.position);
            boundsB.max = Vector3Add(boundsB.max, transformB.position);

            // Check collision
            if (CheckCollisionBoxes(boundsA, boundsB))
            {
                OnCollision(entityA, entityB, collisionA, collisionB);
            }
        }
    }
}

void RenderDebug()
{
    auto view = REGISTRY.view<TransformComponent, CollisionComponent>();

    for (auto &&[entity, transform, collision] : view.each())
    {
        // Calculate world bounds
        BoundingBox bounds = collision.bounds;
        bounds.min = Vector3Add(bounds.min, transform.position);
        bounds.max = Vector3Add(bounds.max, transform.position);

        // Draw wireframe
        Color color = GREEN;
        if (collision.hasCollision)
        {
            color = RED;
        }
        else if (collision.isTrigger)
        {
            color = YELLOW;
        }
        else if (collision.collisionLayer == 0) // Static layer convention
        {
            color = DARKGRAY;
        }

        DrawBoundingBox(bounds, color);
    }
}

} // namespace CollisionSystem
