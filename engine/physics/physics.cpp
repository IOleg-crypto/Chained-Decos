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
                    // Cache AABB for visualization
                    BoundingBox box = AssetManager::GetModelBoundingBox(collider.ModelPath);
                    collider.Offset = box.min;
                    collider.Size = Vector3Subtract(box.max, box.min);
                    CH_CORE_INFO("BVH Built & AABB Cached for entity %d", (uint32_t)entity);
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

            if (rb.UseGravity && !rb.IsGrounded)
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

        // if (rb.IsKinematic)
        //     continue;

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

                    // Apply Scale to RB Collider
                    Vector3 rbScale = transform.Scale;
                    rbcOffset = Vector3Multiply(rbc.Offset, rbScale);
                    Vector3 rbcSize = Vector3Multiply(rbc.Size, rbScale);

                    rbMin = Vector3Add(transform.Translation, rbcOffset);
                    rbMax = Vector3Add(rbMin, rbcSize);
                }

                auto &ot = colliders.get<TransformComponent>(otherEntity);
                // Apply Scale to Other Collider
                Vector3 otherScale = ot.Scale;
                Vector3 otherOffset = Vector3Multiply(oc.Offset, otherScale);
                Vector3 otherSize = Vector3Multiply(oc.Size, otherScale);

                Vector3 otherMin = Vector3Add(ot.Translation, otherOffset);
                Vector3 otherMax = Vector3Add(otherMin, otherSize);

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
                    Vector3 feetPos = transform.Translation;
                    if (registry.all_of<ColliderComponent>(rbEntity))
                    {
                        auto &rbc = registry.get<ColliderComponent>(rbEntity);
                        // Apply Scale to Raycast Offset
                        feetPos = Vector3Add(transform.Translation,
                                             Vector3Multiply(rbc.Offset, transform.Scale));
                    }

                    Ray ray;
                    ray.position = feetPos;
                    ray.position.y += 0.5f; // Start ray slightly above feet
                    ray.direction = {0.0f, -1.0f, 0.0f};

                    // Transform Ray to Model Local Space
                    auto &ot = colliders.get<TransformComponent>(otherEntity);
                    Matrix modelTransform = ot.GetTransform();
                    Matrix invTransform = MatrixInvert(modelTransform);

                    Vector3 localOrigin = Vector3Transform(ray.position, invTransform);
                    Vector3 localTarget =
                        Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
                    Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

                    Ray localRay = {localOrigin, localDir};
                    float t_local = FLT_MAX;
                    Vector3 localNormal;

                    if (BVHBuilder::Raycast(oc.BVHRoot.get(), localRay, t_local, localNormal))
                    {
                        Vector3 hitPosLocal =
                            Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                        Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);

                        float hitY = hitPosWorld.y;
                        float floorThreshold = 0.55f; // Tolerance relative to ray start (0.5)

                        // Only snap if we are close enough to the floor
                        if (hitY >= feetPos.y - 0.1f && hitY <= feetPos.y + floorThreshold)
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
            Vector3 size = Vector3Multiply(collider.Size, transform.Scale);
            Vector3 offset = Vector3Multiply(collider.Offset, transform.Scale);

            box.min = Vector3Add(transform.Translation, offset);
            box.max = Vector3Add(box.min, size);

            RayCollision collision = GetRayCollisionBox(ray, box); // World Space AABB
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
            Matrix modelTransform = transform.GetTransform();
            Matrix invTransform = MatrixInvert(modelTransform);

            Vector3 localOrigin = Vector3Transform(ray.position, invTransform);
            Vector3 localTarget =
                Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
            Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

            Ray localRay = {localOrigin, localDir};
            float t_local = FLT_MAX;
            Vector3 localNormal;

            if (BVHBuilder::Raycast(collider.BVHRoot.get(), localRay, t_local, localNormal))
            {
                // Convert result to World Space
                Vector3 hitPosLocal = Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
                float distWorld = Vector3Distance(ray.position, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;
                    // Normal also needs transform (Rotation only)
                    // Simple approximation: Transform as vector, normalize
                    // For correct normal transform: Inverse Transpose of Model Matrix
                    // But for now, Rotate is likely enough
                    // Matrix rotation = MatrixRotateXYZ(transform.Rotation);
                    // Let's use Vector3Transform with 0 translation
                    // Actually, easiest is Vector3Subtract(Vector3Transform(normal, mat),
                    // Vector3Transform(0, mat))
                    Vector3 normalWorld = Vector3Normalize(
                        Vector3Subtract(Vector3Transform(localNormal, modelTransform),
                                        Vector3Transform({0, 0, 0}, modelTransform)));

                    result.Normal = normalWorld;
                    result.Entity = entity;
                }
            }
        }
    }

    return result;
}
} // namespace CH
