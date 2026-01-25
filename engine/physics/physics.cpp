#include "physics.h"
#include "bvh/bvh.h"
#include "collision/collision.h"
#include "engine/core/profiler.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
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

static void ProcessBoxCollider(ColliderComponent &collider, ModelComponent &model)
{
    if (!collider.bAutoCalculate || model.ModelPath.empty())
        return;

    auto asset = Assets::Get<ModelAsset>(model.ModelPath);
    if (asset)
    {
        BoundingBox box = asset->GetBoundingBox();
        collider.Size = Vector3Subtract(box.max, box.min);
        collider.Offset = box.min;
        collider.bAutoCalculate = false;
    }
}

static void ProcessMeshCollider(ColliderComponent &collider, entt::entity entity)
{
    if (collider.Type != ColliderType::Mesh || collider.ModelPath.empty())
        return;

    auto asset = Assets::Get<ModelAsset>(collider.ModelPath);
    if (!asset || asset->GetModel().meshCount == 0)
        return;

    // Use cached BVH from the model asset
    if (!collider.BVHRoot)
    {
        collider.BVHRoot = asset->GetBVHCache();

        BoundingBox box = asset->GetBoundingBox();
        collider.Offset = box.min;
        collider.Size = Vector3Subtract(box.max, box.min);
        CH_CORE_INFO("Physics: Linked BVH cache for entity {}", (uint32_t)entity);
    }
}

static void ProcessColliderData(entt::registry &sceneRegistry)
{
    auto view = sceneRegistry.view<ColliderComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto &collider = view.get<ColliderComponent>(entity);

        if (collider.Type == ColliderType::Box)
        {
            if (sceneRegistry.all_of<ModelComponent>(entity))
            {
                auto &model = sceneRegistry.get<ModelComponent>(entity);
                ProcessBoxCollider(collider, model);
            }
        }
        else if (collider.Type == ColliderType::Mesh)
        {
            ProcessMeshCollider(collider, entity);
        }
    }
}

static void ApplyRigidBodyPhysics(entt::registry &sceneRegistry,
                                  const std::vector<entt::entity> &entities, float deltaTime,
                                  bool runtime)
{
    float gravity = 20.0f;
    if (Project::GetActive())
        gravity = Project::GetActive()->GetConfig().Physics.Gravity;

    for (size_t i = 0; i < entities.size(); ++i)
    {
        auto entity = entities[i];
        if (!sceneRegistry.all_of<TransformComponent, RigidBodyComponent>(entity))
            continue;

        auto &rigidBody = sceneRegistry.get<RigidBodyComponent>(entity);
        auto &entityTransform = sceneRegistry.get<TransformComponent>(entity);

        if (rigidBody.UseGravity && !rigidBody.IsGrounded && !rigidBody.IsKinematic)
        {
            float oldV = rigidBody.Velocity.y;
            rigidBody.Velocity.y -= gravity * deltaTime;

            if (sceneRegistry.all_of<PlayerComponent>(entity))
            {
                CH_CORE_TRACE("Physics: Player gravity applied: {} -> {}", oldV,
                              rigidBody.Velocity.y);
            }
        }

        // Explicit Player diagnostic
        if (sceneRegistry.all_of<PlayerComponent>(entity))
        {
            static int skipCounter = 0;
            if (skipCounter++ % 120 == 0)
            {
                std::string tag = sceneRegistry.all_of<TagComponent>(entity)
                                      ? sceneRegistry.get<TagComponent>(entity).Tag
                                      : "Unnamed";
                CH_CORE_INFO("Physics State: {} | Grav={} | Grnd={} | Kin={} | V.y={:0.2f}", tag,
                             rigidBody.UseGravity, rigidBody.IsGrounded, rigidBody.IsKinematic,
                             rigidBody.Velocity.y);
            }
        }

        // Apply velocity to translation for ALL bodies
        Vector3 velocityDelta = Vector3Scale(rigidBody.Velocity, deltaTime);
        entityTransform.Translation = Vector3Add(entityTransform.Translation, velocityDelta);
    }
}

static void ResolveCollisionLogic(entt::registry &sceneRegistry,
                                  const std::vector<entt::entity> &rigidBodyEntities)
{
    for (size_t i = 0; i < rigidBodyEntities.size(); ++i)
    {
        auto rbEntity = rigidBodyEntities[i];
        if (!sceneRegistry.all_of<TransformComponent, RigidBodyComponent>(rbEntity))
            continue;

        auto &entityTransform = sceneRegistry.get<TransformComponent>(rbEntity);
        auto &rigidBody = sceneRegistry.get<RigidBodyComponent>(rbEntity);

        // Grounding reset moved to the start of each simulation step
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
                // 1. Calculate RB World Box
                Vector3 rbMin = entityTransform.Translation;
                Vector3 rbMax = entityTransform.Translation;

                if (sceneRegistry.all_of<ColliderComponent>(rbEntity))
                {
                    auto &rbc = sceneRegistry.get<ColliderComponent>(rbEntity);
                    Vector3 rbScale = entityTransform.Scale;
                    Vector3 rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
                    Vector3 rbcSize = Vector3Multiply(rbc.Size, rbScale);
                    rbMin = Vector3Add(entityTransform.Translation, rbColliderOffset);
                    rbMax = Vector3Add(rbMin, rbcSize);
                }

                // 2. Calculate Other World Box
                Vector3 otherScale = otherTransform.Scale;
                Vector3 otherMin = Vector3Add(otherTransform.Translation,
                                              Vector3Multiply(otherCollider.Offset, otherScale));
                Vector3 otherMax =
                    Vector3Add(otherMin, Vector3Multiply(otherCollider.Size, otherScale));

                if (Collision::CheckAABB(rbMin, rbMax, otherMin, otherMax))
                {
                    // Basic separation for boxes (we could use SAT here too, but MTV-Axis is
                    // simpler)
                    float depths[6] = {otherMax.x - rbMin.x, rbMax.x - otherMin.x,
                                       otherMax.y - rbMin.y, rbMax.y - otherMin.y,
                                       otherMax.z - rbMin.z, rbMax.z - otherMin.z};

                    float minDepth = FLT_MAX;
                    int axis = -1;
                    for (int d = 0; d < 6; d++)
                    {
                        if (depths[d] < minDepth)
                        {
                            minDepth = depths[d];
                            axis = d;
                        }
                    }

                    if (axis != -1 && minDepth > 0)
                    {
                        Vector3 mtv = {0, 0, 0};
                        if (axis == 0)
                            mtv.x = minDepth;
                        else if (axis == 1)
                            mtv.x = -minDepth;
                        else if (axis == 2)
                        {
                            mtv.y = minDepth;
                            if (rigidBody.Velocity.y < 0)
                            {
                                rigidBody.IsGrounded = true;
                                rigidBody.Velocity.y = 0;
                            }
                        }
                        else if (axis == 3)
                        {
                            mtv.y = -minDepth;
                            if (rigidBody.Velocity.y > 0)
                                rigidBody.Velocity.y = 0;
                        }
                        else if (axis == 4)
                            mtv.z = minDepth;
                        else if (axis == 5)
                            mtv.z = -minDepth;

                        entityTransform.Translation = Vector3Add(entityTransform.Translation, mtv);
                        otherCollider.IsColliding = true;
                    }
                }
            }
            else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
            {
                // 1. Calculate RB World Box
                Vector3 rbMin = entityTransform.Translation;
                Vector3 rbcSize = {1, 1, 1};
                Vector3 rbColliderOffset = {0, 0, 0};
                if (sceneRegistry.all_of<ColliderComponent>(rbEntity))
                {
                    auto &rbc = sceneRegistry.get<ColliderComponent>(rbEntity);
                    Vector3 rbScale = entityTransform.Scale;
                    rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
                    rbcSize = Vector3Multiply(rbc.Size, rbScale);
                    rbMin = Vector3Add(entityTransform.Translation, rbColliderOffset);
                }

                BoundingBox rbBox = {rbMin, Vector3Add(rbMin, rbcSize)};
                Matrix meshMatrix = otherTransform.GetTransform();
                Matrix invMeshMatrix = MatrixInvert(meshMatrix);

                // World box to local space
                Vector3 corners[8] = {{rbBox.min.x, rbBox.min.y, rbBox.min.z},
                                      {rbBox.max.x, rbBox.min.y, rbBox.min.z},
                                      {rbBox.min.x, rbBox.max.y, rbBox.min.z},
                                      {rbBox.max.x, rbBox.max.y, rbBox.min.z},
                                      {rbBox.min.x, rbBox.min.y, rbBox.max.z},
                                      {rbBox.max.x, rbBox.min.y, rbBox.max.z},
                                      {rbBox.min.x, rbBox.max.y, rbBox.max.z},
                                      {rbBox.max.x, rbBox.max.y, rbBox.max.z}};

                BoundingBox localBox = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
                for (int k = 0; k < 8; k++)
                {
                    Vector3 localCorner = Vector3Transform(corners[k], invMeshMatrix);
                    localBox.min = Vector3Min(localBox.min, localCorner);
                    localBox.max = Vector3Max(localBox.max, localCorner);
                }

                Vector3 localNormal;
                float overlapDepth = -1.0f;
                if (BVHBuilder::IntersectAABB(otherCollider.BVHRoot.get(), localBox, localNormal,
                                              overlapDepth))
                {
                    if (overlapDepth > 0.0001f)
                    {
                        Vector3 localMTV = Vector3Scale(localNormal, overlapDepth);
                        Vector3 worldMTV = Vector3Subtract(Vector3Transform(localMTV, meshMatrix),
                                                           Vector3Transform({0, 0, 0}, meshMatrix));

                        Matrix normalMatrix = MatrixTranspose(invMeshMatrix);
                        Vector3 worldNormal = Vector3Normalize(
                            Vector3Subtract(Vector3Transform(localNormal, normalMatrix),
                                            Vector3Transform({0, 0, 0}, normalMatrix)));

                        entityTransform.Translation =
                            Vector3Add(entityTransform.Translation, worldMTV);

                        if (worldNormal.y > 0.45f)
                        {
                            rigidBody.IsGrounded = true;
                            if (rigidBody.Velocity.y < 0)
                                rigidBody.Velocity.y = 0;
                        }

                        // Velocity reflection/slide
                        float dot = Vector3DotProduct(rigidBody.Velocity, worldNormal);
                        if (dot < -0.01f)
                            rigidBody.Velocity =
                                Vector3Subtract(rigidBody.Velocity, Vector3Scale(worldNormal, dot));

                        otherCollider.IsColliding = true;
                    }
                }
            }
        }
    }
}

void Physics::Update(Scene *scene, float deltaTime, bool runtime)
{
    CH_PROFILE_FUNCTION();
    auto &sceneRegistry = scene->GetRegistry();

    // 0. Update Statistics
    ProfilerStats stats;
    stats.EntityCount = (uint32_t)sceneRegistry.storage<entt::entity>().size();
    stats.ColliderCount = (uint32_t)sceneRegistry.view<ColliderComponent>().size();
    Profiler::UpdateStats(stats);

    // 1. Clear collision flags
    auto collView = sceneRegistry.view<ColliderComponent>();
    for (auto entity : collView)
    {
        collView.get<ColliderComponent>(entity).IsColliding = false;
    }

    // 2. Process Collider Generation (Box AABB & Mesh BVH)
    ProcessColliderData(sceneRegistry);

    if (!runtime)
    {
        return;
    }

    // Collect entities with RigidBody and Transform components for parallel processing
    auto rbView = sceneRegistry.view<TransformComponent, RigidBodyComponent>();
    std::vector<entt::entity> rbEntities;
    rbEntities.reserve(rbView.size_hint());
    for (auto entity : rbView)
        rbEntities.push_back(entity);

    if (rbEntities.empty())
        return;

    // 2. Integration & Collision Resolution
    // Sub-stepping removed as we now use fixed steps in the simulation thread
    ApplyRigidBodyPhysics(sceneRegistry, rbEntities, deltaTime, runtime);
    ResolveCollisionLogic(sceneRegistry, rbEntities);
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
            Vector3 localNormal = {0, 0, 0};
            int localMeshIndex = -1;
            if (BVHBuilder::Raycast(colliderComp.BVHRoot.get(), localRay, t_local, localNormal,
                                    localMeshIndex))
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
                    result.MeshIndex = localMeshIndex;
                }
            }
        }
    }

    return result;
}
} // namespace CHEngine
