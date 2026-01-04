#include "physics.h"
#include "bvh/bvh.h"
#include "collision/collision.h"
#include "engine/scene/components.h"
#include <cfloat>
#include <cmath>
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

            if (Collision::CheckAABB(minA, maxA, minB, maxB))
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
    return Collision::CheckAABB(minA, maxA, minB, maxB);
}

RaycastResult Physics::Raycast(Scene *scene, Ray ray)
{
    RaycastResult result;
    result.Hit = false;
    result.Distance = FLT_MAX;
    result.Entity = entt::null;

    // Box Colliders
    {
        auto view = scene->GetRegistry().view<TransformComponent, BoxColliderComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &collider = view.get<BoxColliderComponent>(entity);

            BoundingBox box;
            box.min = Vector3Add(transform.Translation, collider.Offset);
            box.max = Vector3Add(box.min, collider.Size);

            RayCollision collision = GetRayCollisionBox(ray, box);
            if (collision.hit && collision.distance < result.Distance)
            {
                result.Hit = true;
                result.Distance = collision.distance;
                result.Position = collision.point;
                result.Normal = collision.normal;
                result.Entity = entity;
            }
        }
    }

    // Mesh Colliders
    {
        auto view = scene->GetRegistry().view<TransformComponent, MeshColliderComponent>();
        for (auto entity : view)
        {
            auto &meshCollider = view.get<MeshColliderComponent>(entity);
            if (!meshCollider.Root)
                continue;

            float t = result.Distance;
            Vector3 normal;
            if (BVHBuilder::Raycast(meshCollider.Root.get(), ray, t, normal))
            {
                if (t < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = t;
                    result.Position = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
                    result.Normal = normal;
                    result.Entity = entity;
                }
            }
        }
    }

    return result;
}
} // namespace CH
