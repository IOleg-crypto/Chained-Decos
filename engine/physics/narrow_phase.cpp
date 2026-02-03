#include "narrow_phase.h"
#include "bvh/bvh.h"
#include "cfloat"
#include "collision/collision.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "raymath.h"

namespace CHEngine
{
void NarrowPhase::ResolveCollisions(Scene *scene, const std::vector<::entt::entity> &entities)
{
    auto &registry = scene->GetRegistry();
    auto colliders = registry.view<TransformComponent, ColliderComponent>();

    for (auto rbEntity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent>(rbEntity))
            continue;

        auto &rigidBody = registry.get<RigidBodyComponent>(rbEntity);
        rigidBody.IsGrounded = false; // Reset grounding

        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity)
                continue;

            auto &otherCollider = colliders.get<ColliderComponent>(otherEntity);
            if (!otherCollider.Enabled)
                continue;

            if (otherCollider.Type == ColliderType::Box)
                ResolveBoxBox(registry, rbEntity, otherEntity);
            else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
                ResolveBoxMesh(registry, rbEntity, otherEntity);
        }
    }
}

void NarrowPhase::ResolveBoxBox(::entt::registry &registry, ::entt::entity rbEntity,
                                ::entt::entity otherEntity)
{
    auto &entityTransform = registry.get<TransformComponent>(rbEntity);
    auto &rigidBody = registry.get<RigidBodyComponent>(rbEntity);

    auto &otherTransform = registry.get<TransformComponent>(otherEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);

    // 1. Calculate RB World Box
    Vector3 rbMin = entityTransform.Translation;
    Vector3 rbMax = entityTransform.Translation;

    if (registry.all_of<ColliderComponent>(rbEntity))
    {
        auto &rbc = registry.get<ColliderComponent>(rbEntity);
        Vector3 rbScale = entityTransform.Scale;
        Vector3 rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
        Vector3 rbcSize = Vector3Multiply(rbc.Size, rbScale);
        rbMin = Vector3Add(entityTransform.Translation, rbColliderOffset);
        rbMax = Vector3Add(rbMin, rbcSize);
    }

    // 2. Calculate Other World Box
    Vector3 otherScale = otherTransform.Scale;
    Vector3 otherMin =
        Vector3Add(otherTransform.Translation, Vector3Multiply(otherCollider.Offset, otherScale));
    Vector3 otherMax = Vector3Add(otherMin, Vector3Multiply(otherCollider.Size, otherScale));

    if (Collision::CheckAABB(rbMin, rbMax, otherMin, otherMax))
    {
        float depths[6] = {otherMax.x - rbMin.x, rbMax.x - otherMin.x, otherMax.y - rbMin.y,
                           rbMax.y - otherMin.y, otherMax.z - rbMin.z, rbMax.z - otherMin.z};

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
                if (rigidBody.Velocity.y <= 0.01f)
                {
                    rigidBody.IsGrounded = true;
                    if (rigidBody.Velocity.y < 0)
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

void NarrowPhase::ResolveBoxMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                 ::entt::entity otherEntity)
{
    auto &entityTransform = registry.get<TransformComponent>(rbEntity);
    auto &rigidBody = registry.get<RigidBodyComponent>(rbEntity);

    auto &otherTransform = registry.get<TransformComponent>(otherEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);

    // 1. Calculate RB World Box
    Vector3 rbMin = entityTransform.Translation;
    Vector3 rbcSize = {1, 1, 1};
    Vector3 rbColliderOffset = {0, 0, 0};
    if (registry.all_of<ColliderComponent>(rbEntity))
    {
        auto &rbc = registry.get<ColliderComponent>(rbEntity);
        Vector3 rbScale = entityTransform.Scale;
        rbColliderOffset = Vector3Multiply(rbc.Offset, rbScale);
        rbcSize = Vector3Multiply(rbc.Size, rbScale);
        rbMin = Vector3Add(entityTransform.Translation, rbColliderOffset);
    }

    BoundingBox rbBox = {rbMin, Vector3Add(rbMin, rbcSize)};
    Matrix meshMatrix = otherTransform.GetTransform();
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    // World box to local space
    Vector3 corners[8] = {
        {rbBox.min.x, rbBox.min.y, rbBox.min.z}, {rbBox.max.x, rbBox.min.y, rbBox.min.z},
        {rbBox.min.x, rbBox.max.y, rbBox.min.z}, {rbBox.max.x, rbBox.max.y, rbBox.min.z},
        {rbBox.min.x, rbBox.min.y, rbBox.max.z}, {rbBox.max.x, rbBox.min.y, rbBox.max.z},
        {rbBox.min.x, rbBox.max.y, rbBox.max.z}, {rbBox.max.x, rbBox.max.y, rbBox.max.z}};

    BoundingBox localBox = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    for (int k = 0; k < 8; k++)
    {
        Vector3 localCorner = Vector3Transform(corners[k], invMeshMatrix);
        localBox.min = Vector3Min(localBox.min, localCorner);
        localBox.max = Vector3Max(localBox.max, localCorner);
    }

    Vector3 localNormal;
    float overlapDepth = -1.0f;
    if (otherCollider.BVHRoot->IntersectAABB(localBox, localNormal, overlapDepth))
    {
        if (overlapDepth > 0.0001f)
        {
            Vector3 localMTV = Vector3Scale(localNormal, overlapDepth);
            Vector3 worldMTV = Vector3Subtract(Vector3Transform(localMTV, meshMatrix),
                                               Vector3Transform({0, 0, 0}, meshMatrix));

            Matrix normalMatrix = MatrixTranspose(invMeshMatrix);
            Vector3 worldNormal =
                Vector3Normalize(Vector3Subtract(Vector3Transform(localNormal, normalMatrix),
                                                 Vector3Transform({0, 0, 0}, normalMatrix)));

            entityTransform.Translation = Vector3Add(entityTransform.Translation, worldMTV);

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
} // namespace CHEngine
