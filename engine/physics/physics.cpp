#include "physics.h"
#include "bvh/bvh.h"
#include "collision/collision.h"
#include "engine/renderer/asset_manager.h"
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
    auto &registry = scene->GetRegistry();
    const float GRAVITY_CONSTANT = 20.0f;

    // 1. Process Collider Generation (Box AABB & Mesh BVH)
    {
        auto view = registry.view<ColliderComponent, ModelComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto &collider = view.get<ColliderComponent>(entity);
            if (collider.Type == ColliderType::Box && collider.bAutoCalculate)
            {
                auto &modelComp = view.get<ModelComponent>(entity);
                BoundingBox box = AssetManager::GetModelBoundingBox(modelComp.ModelPath);

                collider.Size = Vector3Subtract(box.max, box.min);
                collider.Offset = box.min;
                collider.bAutoCalculate = false;
            }
            else if (collider.Type == ColliderType::Mesh && !collider.BVHRoot &&
                     !collider.ModelPath.empty())
            {
                Model model = AssetManager::LoadModel(collider.ModelPath);
                if (model.meshCount > 0)
                {
                    collider.BVHRoot = BVHBuilder::Build(model);
                    CH_CORE_INFO("BVH Built for entity %d", (uint32_t)entity);
                }
            }
        }
    }

    // 2. Apply Gravity to RigidBodies
    {
        auto view = registry.view<TransformComponent, RigidBodyComponent>();
        for (auto entity : view)
        {
            auto &rb = view.get<RigidBodyComponent>(entity);
            auto &transform = view.get<TransformComponent>(entity);

            if (rb.UseGravity && !rb.IsGrounded && !rb.IsKinematic)
            {
                rb.Velocity.y -= GRAVITY_CONSTANT * deltaTime;
            }

            // Move
            if (!rb.IsKinematic || Vector3Length(rb.Velocity) > 0.001f)
            {
                Vector3 delta = Vector3Scale(rb.Velocity, deltaTime);
                transform.Translation = Vector3Add(transform.Translation, delta);
            }
        }
    }

    // 3. Collision Resolution & Grounding
    auto rigidBodies = registry.view<TransformComponent, RigidBodyComponent>();
    for (auto rbEntity : rigidBodies)
    {
        auto &transform = rigidBodies.get<TransformComponent>(rbEntity);
        auto &rb = rigidBodies.get<RigidBodyComponent>(rbEntity);

        if (rb.IsKinematic)
            continue;

        rb.IsGrounded = false;

        auto colliders = registry.view<TransformComponent, ColliderComponent>();
        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity)
                continue;

            auto &ot = colliders.get<TransformComponent>(otherEntity);
            auto &oc = colliders.get<ColliderComponent>(otherEntity);

            if (!oc.bEnabled)
                continue;

            if (oc.Type == ColliderType::Box)
            {
                Vector3 rbMin = transform.Translation;
                Vector3 rbMax = transform.Translation;
                Vector3 rbcOffset = {0.0f, 0.0f, 0.0f};

                if (registry.all_of<ColliderComponent>(rbEntity))
                {
                    auto &rbc = registry.get<ColliderComponent>(rbEntity);
                    if (!rbc.bEnabled || rbc.Type != ColliderType::Box)
                        continue;

                    rbcOffset = rbc.Offset;
                    rbMin = Vector3Add(transform.Translation, rbc.Offset);
                    rbMax = Vector3Add(rbMin, rbc.Size);
                }

                Vector3 otherMin = Vector3Add(ot.Translation, oc.Offset);
                Vector3 otherMax = Vector3Add(otherMin, oc.Size);

                if (Collision::CheckAABB(rbMin, rbMax, otherMin, otherMax))
                {
                    if (rb.Velocity.y <= 0.0f && rbMin.y + 0.2f > otherMax.y)
                    {
                        rb.IsGrounded = true;
                        rb.Velocity.y = 0.0f;
                        transform.Translation.y = otherMax.y - rbcOffset.y;
                    }
                }
            }
            else if (oc.Type == ColliderType::Mesh && oc.BVHRoot)
            {
                if (!rb.IsGrounded)
                {
                    Ray ray;
                    ray.position = transform.Translation;
                    ray.position.y += 0.5f;
                    ray.direction = {0.0f, -1.0f, 0.0f};

                    float t = 1.0f;
                    Vector3 normal;

                    if (BVHBuilder::Raycast(oc.BVHRoot.get(), ray, t, normal))
                    {
                        float hitY = ray.position.y - t;
                        float floorThreshold = 0.51f;

                        if (hitY >= transform.Translation.y - 0.1f &&
                            hitY <= transform.Translation.y + floorThreshold)
                        {
                            if (rb.Velocity.y <= 0.0f)
                            {
                                rb.IsGrounded = true;
                                rb.Velocity.y = 0.0f;
                                transform.Translation.y = hitY;
                            }
                        }
                    }
                }
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

    auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &collider = view.get<ColliderComponent>(entity);

        if (!collider.bEnabled)
            continue;

        if (collider.Type == ColliderType::Box)
        {
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
        else if (collider.Type == ColliderType::Mesh && collider.BVHRoot)
        {
            float t = result.Distance;
            Vector3 normal;
            if (BVHBuilder::Raycast(collider.BVHRoot.get(), ray, t, normal))
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
