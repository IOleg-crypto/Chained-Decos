#include "collision_core.h"

#include "bvh/bvh.h"
#include "collision/collision.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/scene/project.h"
#include "physics.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>

namespace CHEngine
{
void CollisionCore::ApplyResponse(entt::registry& registry,
                                  entt::entity rbEntity,
                                  entt::entity otherEntity,
                                  TransformComponent& tc,
                                  RigidBodyComponent& rb,
                                  ColliderComponent& other,
                                  glm::vec3 normal,
                                  float depth)
{
    const float kBaumgarte = 0.8f;
    const float kSlop = 0.005f;
    const float kMaxCorrection = 1.5f;

    float correctionDepth = std::max(depth - kSlop, 0.0f) * kBaumgarte;
    float correction = std::min(correctionDepth, kMaxCorrection);

    if (correction > 0.0f)
    {
        tc.Translation += normal * correction;
        tc.IsDirty = true;

        auto hc = registry.try_get<HierarchyComponent>(rbEntity);
        if (!hc || hc->Parent == entt::null || !registry.valid(hc->Parent))
        {
            tc.WorldTransform = tc.GetTransform();
        }
        else
        {
            tc.WorldTransform = registry.get<TransformComponent>(hc->Parent).WorldTransform * tc.GetTransform();
        }
    }

    if (normal.y > 0.45f)
    {
        rb.IsGrounded = true;
        if (rb.Velocity.y < 0.1f)
        {
            rb.Velocity.y = 0;
        }
        if (std::abs(rb.Velocity.x) < 0.05f) rb.Velocity.x = 0;
        if (std::abs(rb.Velocity.z) < 0.05f) rb.Velocity.z = 0;
    }
    else if (normal.y < -0.45f)
    {
        if (rb.Velocity.y > 0)
        {
            rb.Velocity.y = 0;
        }
    }

    float vDotN = glm::dot(rb.Velocity, normal);
    if (vDotN < 0.0f)
    {
        rb.Velocity -= normal * vDotN;
    }

    other.IsColliding = true;

    if (auto* context = registry.ctx().find<PhysicsContext>())
    {
        if (context->CollisionCallback)
        {
            context->CollisionCallback(rbEntity, otherEntity);
        }
    }
}

glm::vec3 CollisionCore::ClosestPointOnSegment(glm::vec3 p, glm::vec3 a, glm::vec3 b)
{
    glm::vec3 ab = b - a;
    float denom = glm::dot(ab, ab);
    if (denom < 0.0001f)
    {
        return a;
    }
    float t = std::max(0.0f, std::min(1.0f, glm::dot(p - a, ab) / denom));
    return a + ab * t;
}

glm::vec3 CollisionCore::ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;
    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    glm::vec3 bp = p - b;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return a + ab * v;
    }

    glm::vec3 cp = p - c;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return a + ac * w;
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + (c - b) * w;
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return a + ab * v + ac * w;
}

CollisionCore::CapsuleSegment CollisionCore::GetCapsuleSegment(const TransformComponent& tc, const ColliderComponent& cc)
{
    glm::vec3 worldUp = glm::normalize(glm::vec3(tc.WorldTransform[1]));
    glm::vec3 finalPos = glm::vec3(tc.WorldTransform * glm::vec4(cc.Offset, 1.0f));

    float scaleY = glm::length(glm::vec3(tc.WorldTransform[1]));
    float halfSeg = std::max(0.0f, (cc.Height * 0.5f - cc.Radius) * scaleY);

    glm::vec3 a = finalPos - worldUp * halfSeg;
    glm::vec3 b = finalPos + worldUp * halfSeg;

    float worldScaleXZ = std::max(glm::length(glm::vec3(tc.WorldTransform[0])),
                                  glm::length(glm::vec3(tc.WorldTransform[2])));

    return {a, b, cc.Radius * worldScaleXZ};
}

CollisionCore::WorldAABB CollisionCore::GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc)
{
    glm::vec3 minLocal = cc.Offset;
    glm::vec3 maxLocal = cc.Offset + cc.Size;

    glm::vec3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z},
        {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z},
        {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z},
        {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z},
        {maxLocal.x, maxLocal.y, maxLocal.z}};

    glm::vec3 worldMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    glm::vec3 worldMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int i = 0; i < 8; i++)
    {
        glm::vec3 worldPt = glm::vec3(tc.WorldTransform * glm::vec4(corners[i], 1.0f));
        worldMin = glm::min(worldMin, worldPt);
        worldMax = glm::max(worldMax, worldPt);
    }

    return {worldMin, worldMax};
}

void CollisionCore::ResolveCollisions(entt::registry& registry, const std::vector<entt::entity>& entities)
{
    const int kResolveIterations = 4;
    for (int iter = 0; iter < kResolveIterations; iter++)
    {
        for (auto rbEntity : entities)
        {
            if (!registry.all_of<TransformComponent, RigidBodyComponent, ColliderComponent>(rbEntity)) continue;

            if (iter == 0) registry.get<RigidBodyComponent>(rbEntity).IsGrounded = false;

            auto& rbCollider = registry.get<ColliderComponent>(rbEntity);
            auto colliders = registry.view<TransformComponent, ColliderComponent>();
            for (auto otherEntity : colliders)
            {
                if (rbEntity == otherEntity) continue;
                auto& otherCollider = colliders.get<ColliderComponent>(otherEntity);
                if (!otherCollider.Enabled) continue;

                if (otherCollider.Type == ColliderType::Box)
                {
                    if (rbCollider.Type == ColliderType::Box) ResolveBoxBox(registry, rbEntity, otherEntity);
                    else if (rbCollider.Type == ColliderType::Capsule) ResolveCapsuleBox(registry, rbEntity, otherEntity);
                    else if (rbCollider.Type == ColliderType::Sphere) ResolveSphereBox(registry, rbEntity, otherEntity);
                }
                else if (otherCollider.Type == ColliderType::Mesh)
                {
                    if (rbCollider.Type == ColliderType::Box) ResolveBoxMesh(registry, rbEntity, otherEntity);
                    else if (rbCollider.Type == ColliderType::Capsule) ResolveCapsuleMesh(registry, rbEntity, otherEntity);
                    else if (rbCollider.Type == ColliderType::Sphere) ResolveSphereMesh(registry, rbEntity, otherEntity);
                }
                else if (otherCollider.Type == ColliderType::Sphere)
                {
                    if (rbCollider.Type == ColliderType::Sphere) ResolveSphereSphere(registry, rbEntity, otherEntity);
                }
            }
        }
    }
}

void CollisionCore::ResolveBoxBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);

    WorldAABB a = GetWorldAABB(tc, rbc);
    WorldAABB b = GetWorldAABB(registry.get<TransformComponent>(otherEntity), otherCollider);

    if (!Collision::CheckAABB(a.Min, a.Max, b.Min, b.Max)) return;

    float depths[6] = {b.Max.x - a.Min.x,
                       a.Max.x - b.Min.x,
                       b.Max.y - a.Min.y,
                       a.Max.y - b.Min.y,
                       b.Max.z - a.Min.z,
                       a.Max.z - b.Min.z};

    int axis = 0;
    float minDepth = depths[0];
    for (int d = 1; d < 6; d++)
    {
        if (depths[d] < minDepth)
        {
            minDepth = depths[d];
            axis = d;
        }
    }

    if (minDepth <= 0) return;

    const glm::vec3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, dirs[axis], minDepth);
}

void CollisionCore::ResolveBoxMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    if (!Project::GetActive()) return;

    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    glm::mat4 invMeshMatrix = glm::inverse(meshMatrix);
    glm::mat4 localToOtherLocal = invMeshMatrix * tc.WorldTransform;

    glm::vec3 minLocal = rbc.Offset;
    glm::vec3 maxLocal = rbc.Offset + rbc.Size;
    glm::vec3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z},
        {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z},
        {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z},
        {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z},
        {maxLocal.x, maxLocal.y, maxLocal.z}};

    BoundingBox localBox = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    for (int k = 0; k < 8; k++)
    {
        glm::vec3 lc = glm::vec3(localToOtherLocal * glm::vec4(corners[k], 1.0f));
        localBox.Min = glm::min(localBox.Min, lc);
        localBox.Max = glm::max(localBox.Max, lc);
    }

    glm::vec3 localNormal;
    float overlapDepth = -1.0f;
    if (!bvh->IntersectAABB(localBox, localNormal, overlapDepth)) return;
    if (overlapDepth <= 0.0001f) return;

    glm::vec3 worldMTV = glm::vec3(meshMatrix * glm::vec4(localNormal * overlapDepth, 0.0f));
    glm::mat4 normalMatrix = glm::transpose(invMeshMatrix);
    glm::vec3 worldNormal = glm::normalize(glm::vec3(normalMatrix * glm::vec4(localNormal, 0.0f)));

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, worldNormal, glm::length(worldMTV));
}

void CollisionCore::ResolveCapsuleBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& capsule = registry.get<ColliderComponent>(rbEntity);
    auto& box = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    CapsuleSegment seg = GetCapsuleSegment(tc, capsule);
    WorldAABB boxAABB = GetWorldAABB(otherTc, box);

    glm::vec3 boxCenter = (boxAABB.Min + boxAABB.Max) * 0.5f;
    glm::vec3 closestOnSeg = ClosestPointOnSegment(boxCenter, seg.a, seg.b);

    glm::vec3 closestOnBox = glm::clamp(closestOnSeg, boxAABB.Min, boxAABB.Max);

    glm::vec3 finalOnSeg = ClosestPointOnSegment(closestOnBox, seg.a, seg.b);

    glm::vec3 diff = finalOnSeg - closestOnBox;
    float distSq = glm::dot(diff, diff);

    if (distSq >= seg.radius * seg.radius) return;

    float dist = std::sqrt(distSq);
    float penetration = seg.radius - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, box, normal, penetration);
}

void CollisionCore::ResolveCapsuleMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& capsule = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    glm::mat4 invMeshMatrix = glm::inverse(meshMatrix);

    CapsuleSegment seg = GetCapsuleSegment(tc, capsule);

    glm::vec3 localA = glm::vec3(invMeshMatrix * glm::vec4(seg.a, 1.0f));
    glm::vec3 localB = glm::vec3(invMeshMatrix * glm::vec4(seg.b, 1.0f));

    float scaleX = glm::length(glm::vec3(meshMatrix[0]));
    float scaleY = glm::length(glm::vec3(meshMatrix[1]));
    float scaleZ = glm::length(glm::vec3(meshMatrix[2]));

    glm::vec3 localExtents = {(scaleX > 0.0001f) ? seg.radius / scaleX : seg.radius,
                              (scaleY > 0.0001f) ? seg.radius / scaleY : seg.radius,
                              (scaleZ > 0.0001f) ? seg.radius / scaleZ : seg.radius};

    BoundingBox queryBox = {glm::min(localA, localB) - localExtents, glm::max(localA, localB) + localExtents};

    std::vector<const CollisionTriangle*> candidates;
    bvh->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    for (const auto* tri : candidates)
    {
        glm::vec3 v0 = glm::vec3(meshMatrix * glm::vec4(tri->v0, 1.0f));
        glm::vec3 v1 = glm::vec3(meshMatrix * glm::vec4(tri->v1, 1.0f));
        glm::vec3 v2 = glm::vec3(meshMatrix * glm::vec4(tri->v2, 1.0f));

        glm::vec3 triCenter = (v0 + v1 + v2) / 3.0f;
        glm::vec3 segPoint = ClosestPointOnSegment(triCenter, seg.a, seg.b);
        glm::vec3 triPoint = ClosestPointTriangle(segPoint, v0, v1, v2);
        glm::vec3 finalSeg = ClosestPointOnSegment(triPoint, seg.a, seg.b);

        glm::vec3 diff = finalSeg - triPoint;
        float distSq = glm::dot(diff, diff);

        if (distSq >= seg.radius * seg.radius) continue;

        float dist = std::sqrt(distSq);
        float penetration = seg.radius - dist;
        glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : tri->normal;

        if (penetration > 0.0f) ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, normal, penetration);
    }
}

void CollisionCore::ResolveSphereBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& box = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    WorldAABB boxAABB = GetWorldAABB(otherTc, box);
    glm::vec3 sphereWorldPos = glm::vec3(tc.WorldTransform * glm::vec4(sphere.Offset, 1.0f));

    glm::vec3 closestOnBox = glm::clamp(sphereWorldPos, boxAABB.Min, boxAABB.Max);

    glm::vec3 diff = sphereWorldPos - closestOnBox;
    float distSq = glm::dot(diff, diff);

    if (distSq >= sphere.Radius * sphere.Radius) return;

    float dist = std::sqrt(distSq);
    float penetration = sphere.Radius - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, box, normal, penetration);
}

void CollisionCore::ResolveSphereMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    glm::mat4 invMeshMatrix = glm::inverse(meshMatrix);

    glm::vec3 sphereWorldPos = glm::vec3(tc.WorldTransform * glm::vec4(sphere.Offset, 1.0f));
    glm::vec3 sphereLocalPos = glm::vec3(invMeshMatrix * glm::vec4(sphereWorldPos, 1.0f));

    float scaleX = glm::length(glm::vec3(meshMatrix[0]));
    float scaleY = glm::length(glm::vec3(meshMatrix[1]));
    float scaleZ = glm::length(glm::vec3(meshMatrix[2]));

    glm::vec3 localExtents = {(scaleX > 0.0001f) ? sphere.Radius / scaleX : sphere.Radius,
                              (scaleY > 0.0001f) ? sphere.Radius / scaleY : sphere.Radius,
                              (scaleZ > 0.0001f) ? sphere.Radius / scaleZ : sphere.Radius};

    BoundingBox queryBox = {sphereLocalPos - localExtents, sphereLocalPos + localExtents};

    std::vector<const CollisionTriangle*> candidates;
    bvh->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    glm::vec3 bestNormal = {0.0f, 1.0f, 0.0f};
    float maxPenetration = -1.0f;
    bool anyContact = false;

    for (const auto* tri : candidates)
    {
        glm::vec3 v0 = glm::vec3(meshMatrix * glm::vec4(tri->v0, 1.0f));
        glm::vec3 v1 = glm::vec3(meshMatrix * glm::vec4(tri->v1, 1.0f));
        glm::vec3 v2 = glm::vec3(meshMatrix * glm::vec4(tri->v2, 1.0f));

        glm::vec3 triPoint = ClosestPointTriangle(sphereWorldPos, v0, v1, v2);
        glm::vec3 diff = sphereWorldPos - triPoint;
        float distSq = glm::dot(diff, diff);

        if (distSq >= sphere.Radius * sphere.Radius) continue;

        float dist = std::sqrt(distSq);
        float penetration = sphere.Radius - dist;
        glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : tri->normal;

        if (penetration > maxPenetration)
        {
            maxPenetration = penetration;
            bestNormal = normal;
        }
        anyContact = true;
    }

    if (anyContact && maxPenetration > 0.0f)
    {
        ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, bestNormal, maxPenetration);
    }
}

void CollisionCore::ResolveSphereSphere(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity)
{
    auto& tc1 = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& s1 = registry.get<ColliderComponent>(rbEntity);
    auto& s2 = registry.get<ColliderComponent>(otherEntity);
    auto& tc2 = registry.get<TransformComponent>(otherEntity);

    glm::vec3 p1 = glm::vec3(tc1.WorldTransform * glm::vec4(s1.Offset, 1.0f));
    glm::vec3 p2 = glm::vec3(tc2.WorldTransform * glm::vec4(s2.Offset, 1.0f));

    glm::vec3 diff = p1 - p2;
    float distSq = glm::dot(diff, diff);
    float radiusSum = (s1.Radius * glm::length(glm::vec3(tc1.WorldTransform[0]))) +
                      (s2.Radius * glm::length(glm::vec3(tc2.WorldTransform[0])));

    if (distSq >= radiusSum * radiusSum) return;

    float dist = std::sqrt(distSq);
    float penetration = radiusSum - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    ApplyResponse(registry, rbEntity, otherEntity, tc1, rb, s2, normal, penetration);
}

} // namespace CHEngine
