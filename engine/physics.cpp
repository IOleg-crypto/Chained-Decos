#include "physics.h"
#include "components.h"
#include <raymath.h>

namespace CH
{
void Physics::Init()
{
    // Minimalist physics init
}

void Physics::Shutdown()
{
}

void Physics::Update(Scene *scene, float deltaTime)
{
    auto view = scene->GetRegistry().view<TransformComponent, BoxColliderComponent>();

    // Reset collision states
    for (auto entity : view)
    {
        auto &collider = view.get<BoxColliderComponent>(entity);
        collider.IsColliding = false;
    }

    // Check collisions (O(n^2) for now - simple)
    for (auto entityA : view)
    {
        auto &transformA = view.get<TransformComponent>(entityA);
        auto &colliderA = view.get<BoxColliderComponent>(entityA);

        Vector3 minA = Vector3Add(transformA.Translation, colliderA.Offset);
        Vector3 maxA = Vector3Add(minA, colliderA.Size);

        for (auto entityB : view)
        {
            if (entityA == entityB)
                continue;

            auto &transformB = view.get<TransformComponent>(entityB);
            auto &colliderB = view.get<BoxColliderComponent>(entityB);

            Vector3 minB = Vector3Add(transformB.Translation, colliderB.Offset);
            Vector3 maxB = Vector3Add(minB, colliderB.Size);

            if (CheckAABB(minA, maxA, minB, maxB))
            {
                colliderA.IsColliding = true;
                colliderB.IsColliding = true;
            }
        }
    }
}

bool Physics::CheckAABB(const Vector3 &minA, const Vector3 &maxA, const Vector3 &minB,
                        const Vector3 &maxB)
{
    return (minA.x <= maxB.x && maxA.x >= minB.x) && (minA.y <= maxB.y && maxA.y >= minB.y) &&
           (minA.z <= maxB.z && maxA.z >= minB.z);
}
} // namespace CH
