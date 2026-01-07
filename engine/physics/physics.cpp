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
            Model model = AssetManager::LoadModel(colliderComp.ModelPath);
            if (model.meshCount > 0)
            {
                colliderComp.BVHRoot = BVHBuilder::Build(model);
                BoundingBox box = AssetManager::GetModelBoundingBox(colliderComp.ModelPath);
                colliderComp.Offset = box.min;
                colliderComp.Size = Vector3Subtract(box.max, box.min);
                CH_CORE_INFO("BVH Built & AABB Cached for entity %d", (uint32_t)entity);
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
                    Vector3 feetPosition = entityTransform.Translation;
                    if (sceneRegistry.all_of<ColliderComponent>(rbEntity))
                    {
                        auto &rbc = sceneRegistry.get<ColliderComponent>(rbEntity);
                        feetPosition =
                            Vector3Add(entityTransform.Translation,
                                       Vector3Multiply(rbc.Offset, entityTransform.Scale));
                    }

                    Ray groundingRay;
                    groundingRay.position = feetPosition;
                    groundingRay.position.y += 0.5f;
                    groundingRay.direction = {0.0f, -1.0f, 0.0f};

                    Matrix modelTransform = otherTransform.GetTransform();
                    Matrix invTransform = MatrixInvert(modelTransform);

                    Vector3 localOrigin = Vector3Transform(groundingRay.position, invTransform);
                    Vector3 localTarget = Vector3Transform(
                        Vector3Add(groundingRay.position, groundingRay.direction), invTransform);
                    Vector3 localDir = Vector3Normalize(Vector3Subtract(localTarget, localOrigin));

                    Ray localRay = {localOrigin, localDir};
                    float t_local = FLT_MAX;
                    Vector3 localNormal;

                    if (BVHBuilder::Raycast(otherCollider.BVHRoot.get(), localRay, t_local,
                                            localNormal))
                    {
                        Vector3 hitPosLocal =
                            Vector3Add(localOrigin, Vector3Scale(localDir, t_local));
                        Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);

                        float hitY = hitPosWorld.y;
                        float floorThreshold = 0.55f;

                        if (hitY >= feetPosition.y - 0.1f &&
                            hitY <= feetPosition.y + floorThreshold)
                        {
                            if (rigidBody.Velocity.y <= 0.0f)
                            {
                                rigidBody.IsGrounded = true;
                                rigidBody.Velocity.y = 0.0f;
                                entityTransform.Translation.y = hitY;
                                otherCollider.IsColliding = true;
                            }
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

    // 2. Apply Gravity & Movement
    ApplyRigidBodyPhysics(sceneRegistry, deltaTime);

    // 3. Collision Resolution & Grounding
    ResolveCollisionLogic(sceneRegistry);
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
} // namespace CH
