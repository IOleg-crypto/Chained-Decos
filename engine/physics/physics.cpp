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

    // 1. Auto-Calculate Box Colliders
    {
        auto view = registry.view<BoxColliderComponent, ModelComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto &collider = view.get<BoxColliderComponent>(entity);
            if (collider.bAutoCalculate)
            {
                auto &modelComp = view.get<ModelComponent>(entity);
                BoundingBox box = AssetManager::GetModelBoundingBox(modelComp.ModelPath);

                collider.Size = Vector3Subtract(box.max, box.min);
                collider.Offset = box.min;
                collider.bAutoCalculate = false;
            }
        }
    }

    // 2. Auto-Build Mesh Colliders (BVH)
    {
        auto view = registry.view<MeshColliderComponent, TransformComponent>();
        for (auto entity : view)
        {
            auto &meshCollider = view.get<MeshColliderComponent>(entity);
            if (!meshCollider.Root && !meshCollider.ModelPath.empty())
            {
                Model model = AssetManager::LoadModel(meshCollider.ModelPath);
                if (model.meshCount > 0)
                {
                    meshCollider.Root = BVHBuilder::Build(model);
                    CH_CORE_INFO("BVH Built for entity %d", (uint32_t)entity);
                }
            }
        }
    }

    // 3. Apply Gravity to RigidBodies
    {
        auto view = registry.view<TransformComponent, RigidBodyComponent>();
        for (auto entity : view)
        {
            auto &rb = view.get<RigidBodyComponent>(entity);
            auto &transform = view.get<TransformComponent>(entity);

            if (rb.UseGravity && !rb.IsGrounded)
            {
                rb.Velocity.y -= GRAVITY_CONSTANT * deltaTime;
            }

            // Tentative movement
            Vector3 delta = Vector3Scale(rb.Velocity, deltaTime);
            transform.Translation = Vector3Add(transform.Translation, delta);
        }
    }

    // 4. Collision Resolution & Grounding
    auto rigidBodies = registry.view<TransformComponent, RigidBodyComponent>();
    for (auto rbEntity : rigidBodies)
    {
        auto &transform = rigidBodies.get<TransformComponent>(rbEntity);
        auto &rb = rigidBodies.get<RigidBodyComponent>(rbEntity);

        rb.IsGrounded = false;

        // A. Check against Box Colliders
        auto boxColliders = registry.view<TransformComponent, BoxColliderComponent>();
        for (auto boxEntity : boxColliders)
        {
            if (rbEntity == boxEntity)
                continue;

            auto &bt = boxColliders.get<TransformComponent>(boxEntity);
            auto &bc = boxColliders.get<BoxColliderComponent>(boxEntity);

            Vector3 rbMin = transform.Translation; // Simplified
            Vector3 rbMax = transform.Translation;
            if (registry.all_of<BoxColliderComponent>(rbEntity))
            {
                auto &rbc = registry.get<BoxColliderComponent>(rbEntity);
                rbMin = Vector3Add(transform.Translation, rbc.Offset);
                rbMax = Vector3Add(rbMin, rbc.Size);
            }

            Vector3 otherMin = Vector3Add(bt.Translation, bc.Offset);
            Vector3 otherMax = Vector3Add(otherMin, bc.Size);

            if (Collision::CheckAABB(rbMin, rbMax, otherMin, otherMax))
            {
                if (rb.Velocity.y <= 0.0f && rbMax.y > otherMax.y)
                {
                    rb.IsGrounded = true;
                    rb.Velocity.y = 0.0f;

                    float rbcOffset = 0.0f;
                    if (registry.all_of<BoxColliderComponent>(rbEntity))
                        rbcOffset = registry.get<BoxColliderComponent>(rbEntity).Offset.y;

                    transform.Translation.y = otherMax.y - rbcOffset;
                }
            }
        }

        // B. Check against Mesh Colliders (BVH) for stairs/terrain
        if (!rb.IsGrounded)
        {
            auto meshColliders = registry.view<TransformComponent, MeshColliderComponent>();
            for (auto meshEntity : meshColliders)
            {
                auto &mt = meshColliders.get<TransformComponent>(meshEntity);
                auto &mc = meshColliders.get<MeshColliderComponent>(meshEntity);

                if (mc.Root)
                {
                    // Raycast down from slightly above the bottom to detect floor
                    Ray ray;
                    ray.position = transform.Translation;
                    ray.position.y += 0.5f; // Start ray inside/above
                    ray.direction = {0.0f, -1.0f, 0.0f};

                    float t = 1.0f; // Max distance to check
                    Vector3 normal;

                    // Transform ray to local space of the mesh if it has rotation/scale (not
                    // supported by current BVHBuilder yet) For now assume static meshes at world
                    // space or identity transform

                    if (BVHBuilder::Raycast(mc.Root.get(), ray, t, normal))
                    {
                        float hitY = ray.position.y - t;
                        float floorThreshold = 0.1f;

                        // If hit is close to our current foot position
                        if (hitY >= transform.Translation.y - floorThreshold &&
                            hitY <= transform.Translation.y + 0.5f)
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
