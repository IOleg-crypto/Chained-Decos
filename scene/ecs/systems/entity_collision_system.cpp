#include "entity_collision_system.h"
#include "scene/ecs/components/physics_data.h"
#include "scene/ecs/components/transform_component.h"
#include <raylib.h>

namespace CHEngine
{
void EntityCollisionSystem::Update(entt::registry &registry, float deltaTime)
{
    auto view = registry.view<TransformComponent, CollisionComponent>();

    for (auto &&[entityA, transformA, collisionA] : view.each())
    {
        collisionA.hasCollision = false;
        collisionA.collidedWith = entt::null;

        for (auto &&[entityB, transformB, collisionB] : view.each())
        {
            if (entityA == entityB)
                continue;

            // Layer/Mask Check
            if (!(collisionA.collisionMask & (1 << collisionB.collisionLayer)))
                continue;

            BoundingBox bA = collisionA.bounds;
            bA.min = Vector3Add(bA.min, transformA.position);
            bA.max = Vector3Add(bA.max, transformA.position);

            BoundingBox bB = collisionB.bounds;
            bB.min = Vector3Add(bB.min, transformB.position);
            bB.max = Vector3Add(bB.max, transformB.position);

            if (CheckCollisionBoxes(bA, bB))
            {
                collisionA.hasCollision = true;
                collisionA.collidedWith = entityB;

                // Note: We don't set collisionB here because it will be checked in its own
                // iteration or we can set it for optimization, but for simplicity let's keep it
                // symmetrical.
            }
        }
    }
}
} // namespace CHEngine
