#include "CollisionSystem.h"
#include "core/ecs/Components/PhysicsComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/ECSRegistry.h"
#include <raylib.h>
#include <raymath.h>


namespace CollisionSystem
{

static void OnCollision(entt::entity a, entt::entity b, CollisionComponent &collA,
                        CollisionComponent &collB)
{
    // Встановити стан зіткнення
    collA.hasCollision = true;
    collA.collidedWith = b;

    collB.hasCollision = true;
    collB.collidedWith = a;

    // Якщо це trigger - не блокувати рух
    if (collA.isTrigger || collB.isTrigger)
    {
        return;
    }

    // Тут можна додати розв'язання зіткнень (collision response)
    // Наприклад, відштовхнути entities один від одного
}

void Update()
{
    auto view = REGISTRY.view<TransformComponent, CollisionComponent>();

    // Broad phase - швидка перевірка
    // Narrow phase - точна перевірка

    for (auto [entityA, transformA, collisionA] : view.each())
    {
        // Скинути стан зіткнення
        collisionA.hasCollision = false;
        collisionA.collidedWith = entt::null;

        for (auto [entityB, transformB, collisionB] : view.each())
        {
            if (entityA == entityB)
                continue;

            // Перевірка collision mask
            if (!(collisionA.collisionMask & (1 << collisionB.collisionLayer)))
            {
                continue;
            }

            // Оновити bounds
            BoundingBox boundsA = collisionA.bounds;
            boundsA.min = Vector3Add(boundsA.min, transformA.position);
            boundsA.max = Vector3Add(boundsA.max, transformA.position);

            BoundingBox boundsB = collisionB.bounds;
            boundsB.min = Vector3Add(boundsB.min, transformB.position);
            boundsB.max = Vector3Add(boundsB.max, transformB.position);

            // Перевірка зіткнення
            if (CheckCollisionBoxes(boundsA, boundsB))
            {
                OnCollision(entityA, entityB, collisionA, collisionB);
            }
        }
    }
}

} // namespace CollisionSystem
