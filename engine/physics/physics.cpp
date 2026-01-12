#include "physics.h"
#include "bvh/bvh.h"
#include "collision/collision.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include <cfloat>
#include <chrono>
#include <cmath>
#include <raymath.h>

namespace CHEngine
{
void Physics::Init()
{
}

void Physics::Shutdown()
{
}

static void ProcessColliderData(entt::registry &sceneRegistry)
{
    auto view = sceneRegistry.view<ColliderComponent, ModelComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto &colliderComp = view.get<ColliderComponent>(entity);
        if (colliderComp.Type == ColliderType::Box && colliderComp.bAutoCalculate)
        {
            auto &modelComp = view.get<ModelComponent>(entity);
            BoundingBox box = AssetManager::GetModelBoundingBox(modelComp.ModelPath);

            colliderComp.Size = Vector3Subtract(box.max, box.min);
            colliderComp.Offset = box.min;
            colliderComp.bAutoCalculate = false;
        }
        else if (colliderComp.Type == ColliderType::Mesh && !colliderComp.BVHRoot &&
                 !colliderComp.ModelPath.empty())
        {
            // Load model async if not cached
            if (!AssetManager::IsModelReady(colliderComp.ModelPath))
            {
                AssetManager::LoadModelAsync(colliderComp.ModelPath);
                continue; // Will process next frame when ready
            }

            Model model = AssetManager::LoadModel(colliderComp.ModelPath);
            if (model.meshCount > 0)
            {
                // Start building BVH asynchronously if not already building
                if (!colliderComp.BVHFuture.valid())
                {
                    colliderComp.BVHFuture = BVHBuilder::BuildAsync(model).share();
                    CH_CORE_INFO("Started BVH Build Async for entity %d", (uint32_t)entity);
                }

                // Check if future is ready
                if (colliderComp.BVHFuture.wait_for(std::chrono::seconds(0)) ==
                    std::future_status::ready)
                {
                    colliderComp.BVHRoot = colliderComp.BVHFuture.get();

                    BoundingBox box = AssetManager::GetModelBoundingBox(colliderComp.ModelPath);
                    colliderComp.Offset = box.min;
                    colliderComp.Size = Vector3Subtract(box.max, box.min);
                    CH_CORE_INFO("BVH Build Finished for entity %d", (uint32_t)entity);
                }
            }
        }
    }
}

static void ApplyRigidBodyPhysics(entt::registry &sceneRegistry, float deltaTime)
{
    const float GRAVITY_CONSTANT = 20.0f;
    auto view = sceneRegistry.view<TransformComponent, RigidBodyComponent>();
    for (auto entity : view)
    {
        auto &rigidBody = view.get<RigidBodyComponent>(entity);
        auto &entityTransform = view.get<TransformComponent>(entity);

        if (rigidBody.UseGravity && !rigidBody.IsGrounded)
        {
            rigidBody.Velocity.y -= GRAVITY_CONSTANT * deltaTime;
        }

        if (!rigidBody.IsKinematic || Vector3Length(rigidBody.Velocity) > 0.001f)
        {
            Vector3 velocityDelta = Vector3Scale(rigidBody.Velocity, deltaTime);

            // Basic CCD for vertical movement (floor penetration prevention)
            if (rigidBody.Velocity.y < 0.0f)
            {
                // Raycast downwards from current position to expected next position
                Ray ray = {entityTransform.Translation, {0.0f, -1.0f, 0.0f}};
                float rayDist = -velocityDelta.y + 0.2f; // Check slightly further than the delta

                // We need to access a raycast function that works with the registry or scene
                // Let's use a simple per-mesh check for now if available, or just proceed with
                // sub-stepping
            }

            entityTransform.Translation = Vector3Add(entityTransform.Translation, velocityDelta);
        }
    }
}

static void ResolveCollisionLogic(entt::registry &sceneRegistry)
{
    auto rigidBodies = sceneRegistry.view<TransformComponent, RigidBodyComponent>();
    for (auto rbEntity : rigidBodies)
    {
        auto &entityTransform = rigidBodies.get<TransformComponent>(rbEntity);
        auto &rigidBody = rigidBodies.get<RigidBodyComponent>(rbEntity);

        rigidBody.IsGrounded = false;

        auto colliders = sceneRegistry.view<TransformComponent, ColliderComponent>();
        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity)
                continue;

            auto &otherTransform = colliders.get<TransformComponent>(otherEntity);
            auto &otherCollider = colliders.get<ColliderComponent>(otherEntity);

            if (!otherCollider.bEnabled)
                continue;

            if (otherCollider.Type == ColliderType::Box)
            {
                Vector3 rigidBodyMin = entityTransform.Translation;
                Vector3 rigidBodyMax = entityTransform.Translation;
                Vector3 rbColliderOffset = {0.0f, 0.0f, 0.0f};

                if (sceneRegistry.all_of<ColliderComponent>(rbEntity))
                {
                    auto &rbc = sceneRegistry.get<ColliderComponent>(rbEntity);
                    if (!rbc.bEnabled || rbc.Type != ColliderType::Box)
                        continue;

                    Vector3 rbScale = entityTransform.Scale;
                    rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
                    Vector3 rbcSize = Vector3Multiply(rbc.Size, rbScale);

                    rigidBodyMin = Vector3Add(entityTransform.Translation, rbColliderOffset);
                    rigidBodyMax = Vector3Add(rigidBodyMin, rbcSize);
                }

                Vector3 otherScale = otherTransform.Scale;
                Vector3 otherMin = Vector3Add(otherTransform.Translation,
                                              Vector3Multiply(otherCollider.Offset, otherScale));
                Vector3 otherMax =
                    Vector3Add(otherMin, Vector3Multiply(otherCollider.Size, otherScale));

                if (Collision::CheckAABB(rigidBodyMin, rigidBodyMax, otherMin, otherMax))
                {
                    if (rigidBody.Velocity.y <= 0.0f && rigidBodyMin.y + 0.2f > otherMax.y)
                    {
                        rigidBody.IsGrounded = true;
                        rigidBody.Velocity.y = 0.0f;
                        entityTransform.Translation.y = otherMax.y - rbColliderOffset.y;
                        otherCollider.IsColliding = true;
                    }
                }
            }
            else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
            {
                if (!rigidBody.IsGrounded)
                {
                    Vector3 rbScale = entityTransform.Scale;
                    Vector3 rbcSize = {1.0f, 1.0f, 1.0f};
                    Vector3 rbColliderOffset = {0, 0, 0};

                    if (sceneRegistry.all_of<ColliderComponent>(rbEntity))
                    {
                        auto &rbc = sceneRegistry.get<ColliderComponent>(rbEntity);
                        rbcSize = Vector3Multiply(rbc.Size, rbScale);
                        rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
                    }

                    BoundingBox rbBox;
                    rbBox.min = Vector3Add(entityTransform.Translation, rbColliderOffset);
                    rbBox.max = Vector3Add(rbBox.min, rbcSize);

                    Vector3 overlapNormal;
                    float overlapDepth;

                    if (BVHBuilder::IntersectAABB(otherCollider.BVHRoot.get(), rbBox, overlapNormal,
                                                  overlapDepth))
                    {
                        if (overlapDepth > 0.001f)
                        {
                            // Resolution
                            entityTransform.Translation =
                                Vector3Add(entityTransform.Translation,
                                           Vector3Scale(overlapNormal, overlapDepth));

                            // Grounding check based on normal
                            if (overlapNormal.y > 0.5f)
                            {
                                rigidBody.IsGrounded = true;
                                if (rigidBody.Velocity.y < 0)
                                    rigidBody.Velocity.y = 0;
                            }

                            // Sliding
                            float dot = Vector3DotProduct(rigidBody.Velocity, overlapNormal);
                            if (dot < 0)
                            {
                                rigidBody.Velocity = Vector3Subtract(
                                    rigidBody.Velocity, Vector3Scale(overlapNormal, dot));
                            }

                            otherCollider.IsColliding = true;
                        }
                    }
                }
            }
        }
    }
}

void Physics::Update(Scene *scene, float deltaTime, bool runtime)
{
    auto &sceneRegistry = scene->GetRegistry();

    // 0. Clear collision flags
    auto collView = sceneRegistry.view<ColliderComponent>();
    for (auto entity : collView)
    {
        collView.get<ColliderComponent>(entity).IsColliding = false;
    }

    // 1. Process Collider Generation (Box AABB & Mesh BVH)
    ProcessColliderData(sceneRegistry);

    if (!runtime)
        return;

    // 2. Sub-stepping for better accuracy
    const int numSubSteps = 4;
    float subDeltaTime = deltaTime / (float)numSubSteps;

    for (int i = 0; i < numSubSteps; i++)
    {
        // 3. Apply Gravity & Movement
        ApplyRigidBodyPhysics(sceneRegistry, subDeltaTime);

        // 4. Collision Resolution & Grounding
        ResolveCollisionLogic(sceneRegistry);
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
        auto &entityTransform = view.get<TransformComponent>(entity);
        auto &colliderComp = view.get<ColliderComponent>(entity);

        if (!colliderComp.bEnabled)
            continue;

        if (colliderComp.Type == ColliderType::Box)
        {
            BoundingBox box;
            Vector3 scaledSize = Vector3Multiply(colliderComp.Size, entityTransform.Scale);
            Vector3 scaledOffset = Vector3Multiply(colliderComp.Offset, entityTransform.Scale);

            box.min = Vector3Add(entityTransform.Translation, scaledOffset);
            box.max = Vector3Add(box.min, scaledSize);

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
        else if (colliderComp.Type == ColliderType::Mesh && colliderComp.BVHRoot)
        {
            Matrix modelTransform = entityTransform.GetTransform();
            Matrix invTransform = MatrixInvert(modelTransform);

            Vector3 localOrigin = Vector3Transform(ray.position, invTransform);
            Vector3 localTarget =
                Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
            Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

            Ray localRay = {localOrigin, localDir};
            float t_local = FLT_MAX;
            Vector3 localNormal;

            if (BVHBuilder::Raycast(colliderComp.BVHRoot.get(), localRay, t_local, localNormal))
            {
                Vector3 hitPosLocal = Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
                float distWorld = Vector3Distance(ray.position, hitPosWorld);

                if (distWorld < result.Distance)
                {
                    result.Hit = true;
                    result.Distance = distWorld;
                    result.Position = hitPosWorld;

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
} // namespace CHEngine
