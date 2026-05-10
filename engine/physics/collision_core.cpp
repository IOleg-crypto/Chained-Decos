#include "collision_core.h"


#include "bvh/bvh.h"
#include "collision/collision.h"
#include "physics.h"
#include "engine/scene/components/component_utils.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>

namespace CHEngine
{
glm::vec3 CollisionCore::ClosestPointOnSegment(glm::vec3 p, glm::vec3 a, glm::vec3 b)
{
    // Standard closest-point helper used by capsule and mesh contact tests.
    glm::vec3 ab = b - a;
    float denom = glm::dot(ab, ab);
    if (denom < 0.0001f)
    {
        return a;
    }
    float segmentT = std::max(0.0f, std::min(1.0f, glm::dot(p - a, ab) / denom));
    return a + ab * segmentT;
}

glm::vec3 CollisionCore::ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    // Standard closest-point-on-triangle test from Real-Time Collision Detection.
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ap = p - a;
    float abDotAp = glm::dot(ab, ap);
    float acDotAp = glm::dot(ac, ap);
    if (abDotAp <= 0.0f && acDotAp <= 0.0f)
    {
        return a;
    }

    glm::vec3 bp = p - b;
    float abDotBp = glm::dot(ab, bp);
    float acDotBp = glm::dot(ac, bp);
    if (abDotBp >= 0.0f && acDotBp <= abDotBp)
    {
        return b;
    }

    float edgeVC = abDotAp * acDotBp - abDotBp * acDotAp;
    if (edgeVC <= 0.0f && abDotAp >= 0.0f && abDotBp <= 0.0f)
    {
        float barycentricV = abDotAp / (abDotAp - abDotBp);
        return a + ab * barycentricV;
    }

    glm::vec3 cp = p - c;
    float abDotCp = glm::dot(ab, cp);
    float acDotCp = glm::dot(ac, cp);
    if (acDotCp >= 0.0f && abDotCp <= acDotCp)
    {
        return c;
    }

    float edgeVB = abDotCp * acDotAp - abDotAp * acDotCp;
    if (edgeVB <= 0.0f && acDotAp >= 0.0f && acDotCp <= 0.0f)
    {
        float barycentricW = acDotAp / (acDotAp - acDotCp);
        return a + ac * barycentricW;
    }

    float edgeVA = abDotBp * acDotCp - abDotCp * acDotBp;
    if (edgeVA <= 0.0f && (acDotBp - abDotBp) >= 0.0f && (abDotCp - acDotCp) >= 0.0f)
    {
        float barycentricW = (acDotBp - abDotBp) / ((acDotBp - abDotBp) + (abDotCp - acDotCp));
        return b + (c - b) * barycentricW;
    }

    float inverseDenominator = 1.0f / (edgeVA + edgeVB + edgeVC);
    float barycentricV = edgeVB * inverseDenominator;
    float barycentricW = edgeVC * inverseDenominator;
    return a + ab * barycentricV + ac * barycentricW;
}

CollisionCore::CapsuleSegment CollisionCore::GetCapsuleSegment(const TransformComponent& tc,
                                                               const ColliderComponent& cc)
{
    // Scale the capsule with the world transform so non-uniform scaling stays consistent.
    glm::vec3 worldUp = glm::normalize(glm::vec3(tc.WorldTransform[1]));
    glm::vec3 finalPos = glm::vec3(tc.WorldTransform * glm::vec4(cc.Offset, 1.0f));

    float scaleY = glm::length(glm::vec3(tc.WorldTransform[1]));
    float halfSeg = std::max(0.0f, (cc.Height * 0.5f - cc.Radius) * scaleY);

    glm::vec3 a = finalPos - worldUp * halfSeg;
    glm::vec3 b = finalPos + worldUp * halfSeg;

    float worldScaleXZ =
        std::max(glm::length(glm::vec3(tc.WorldTransform[0])), glm::length(glm::vec3(tc.WorldTransform[2])));

    return {a, b, cc.Radius * worldScaleXZ};
}

CollisionCore::WorldAABB CollisionCore::GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc)
{
    if (cc.Type == ColliderType::Sphere)
    {
        // World-space center + uniform radius scaled by max axis scale
        glm::vec3 center = glm::vec3(tc.WorldTransform * glm::vec4(cc.Offset, 1.0f));
        float scale = std::max({glm::length(glm::vec3(tc.WorldTransform[0])),
                                glm::length(glm::vec3(tc.WorldTransform[1])),
                                glm::length(glm::vec3(tc.WorldTransform[2]))});
        float worldRadius = cc.Radius * scale;
        return {center - worldRadius, center + worldRadius};
    }

    if (cc.Type == ColliderType::Capsule)
    {
        // Derive AABB from the actual capsule segment + radius
        CapsuleSegment seg = GetCapsuleSegment(tc, cc);
        glm::vec3 minPt = glm::min(seg.a, seg.b) - seg.radius;
        glm::vec3 maxPt = glm::max(seg.a, seg.b) + seg.radius;
        return {minPt, maxPt};
    }

    if (cc.Type == ColliderType::Mesh)
    {
        // Use the BVH root node's local AABB transformed to world space.
        // This is tight and accurate, preventing false positives in broadphase.
        if (!cc.ModelPath.empty())
        {
            auto bvh = Physics::GetBVH(cc.ModelPath);
            if (bvh && !bvh->GetNodes().empty())
            {
                const auto& root = bvh->GetNodes()[0];
                
                // Safety check for root nodes that might be uninitialized or extreme
                if (std::isfinite(root.AABBMin.x) && std::isfinite(root.AABBMax.x) &&
                    root.AABBMax.x > root.AABBMin.x - 1000.0f && root.AABBMax.x < root.AABBMin.x + 1000.0f)
                {
                    const glm::vec3 corners[8] = {
                        {root.AABBMin.x, root.AABBMin.y, root.AABBMin.z}, {root.AABBMax.x, root.AABBMin.y, root.AABBMin.z},
                        {root.AABBMin.x, root.AABBMax.y, root.AABBMin.z}, {root.AABBMax.x, root.AABBMax.y, root.AABBMin.z},
                        {root.AABBMin.x, root.AABBMin.y, root.AABBMax.z}, {root.AABBMax.x, root.AABBMin.y, root.AABBMax.z},
                        {root.AABBMin.x, root.AABBMax.y, root.AABBMax.z}, {root.AABBMax.x, root.AABBMax.y, root.AABBMax.z}};

                    glm::vec3 worldMin = {FLT_MAX, FLT_MAX, FLT_MAX};
                    glm::vec3 worldMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
                    for (const auto& c : corners)
                    {
                        glm::vec3 wp = glm::vec3(tc.WorldTransform * glm::vec4(c, 1.0f));
                        worldMin = glm::min(worldMin, wp);
                        worldMax = glm::max(worldMax, wp);
                    }
                    return {worldMin, worldMax};
                }
            }
        }
        // Fallback if BVH not yet loaded or invalid: use transform center + small radius
        glm::vec3 center = glm::vec3(tc.WorldTransform[3]);
        return {center - 0.5f, center + 0.5f};
    }

    // Box (default): transform all 8 local corners to world space
    glm::vec3 minLocal = cc.Offset;
    glm::vec3 maxLocal = cc.Offset + cc.Size;

    const glm::vec3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}};

    glm::vec3 worldMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    glm::vec3 worldMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (const auto& c : corners)
    {
        glm::vec3 wp = glm::vec3(tc.WorldTransform * glm::vec4(c, 1.0f));
        worldMin = glm::min(worldMin, wp);
        worldMax = glm::max(worldMax, wp);
    }
    return {worldMin, worldMax};
}


void CollisionCore::ResolveBoxBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);

    WorldAABB a = GetWorldAABB(tc, rbc);
    WorldAABB b = GetWorldAABB(registry.get<TransformComponent>(otherEntity), otherCollider);

    if (!Collision::CheckAABB(a.Min, a.Max, b.Min, b.Max))
    {
        return;
    }

    float depths[6] = {b.Max.x - a.Min.x, a.Max.x - b.Min.x, b.Max.y - a.Min.y,
                       a.Max.y - b.Min.y, b.Max.z - a.Min.z, a.Max.z - b.Min.z};

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

    if (minDepth <= 0)
    {
        return;
    }

    const glm::vec3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    contacts.push_back({rbEntity, otherEntity, dirs[axis], minDepth});
}

void CollisionCore::ResolveBoxMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty())
    {
        return;
    }
    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh)
    {
        return;
    }

    WorldAABB worldAABB = GetWorldAABB(tc, rbc);
    glm::mat4 invMeshMatrix = otherTc.InverseWorldTransform;

    BoundingBox localBoxInMesh = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    glm::vec3 corners[8] = {
        {worldAABB.Min.x, worldAABB.Min.y, worldAABB.Min.z}, {worldAABB.Max.x, worldAABB.Min.y, worldAABB.Min.z},
        {worldAABB.Min.x, worldAABB.Max.y, worldAABB.Min.z}, {worldAABB.Max.x, worldAABB.Max.y, worldAABB.Min.z},
        {worldAABB.Min.x, worldAABB.Min.y, worldAABB.Max.z}, {worldAABB.Max.x, worldAABB.Min.y, worldAABB.Max.z},
        {worldAABB.Min.x, worldAABB.Max.y, worldAABB.Max.z}, {worldAABB.Max.x, worldAABB.Max.y, worldAABB.Max.z}};
    for (int k = 0; k < 8; k++)
    {
        glm::vec3 lc = glm::vec3(invMeshMatrix * glm::vec4(corners[k], 1.0f));
        localBoxInMesh.Min = glm::min(localBoxInMesh.Min, lc);
        localBoxInMesh.Max = glm::max(localBoxInMesh.Max, lc);
    }

    std::vector<const CollisionTriangle*> candidates;
    bvh->QueryAABB(localBoxInMesh, candidates);

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    
    glm::vec3 bestNormal = {0.0f, 1.0f, 0.0f};
    float maxPenetration = -1.0f;

    for (const auto* tri : candidates)
    {
        glm::vec3 v[3] = {glm::vec3(meshMatrix * glm::vec4(tri->v0, 1.0f)),
                          glm::vec3(meshMatrix * glm::vec4(tri->v1, 1.0f)),
                          glm::vec3(meshMatrix * glm::vec4(tri->v2, 1.0f))};

        glm::vec3 boxWorldCenter = glm::vec3(tc.WorldTransform * glm::vec4(rbc.Offset + rbc.Size * 0.5f, 1.0f));

        glm::vec3 triPoint = ClosestPointTriangle(boxWorldCenter, v[0], v[1], v[2]);
        glm::vec3 diff = boxWorldCenter - triPoint;
        float distSq = glm::dot(diff, diff);
        float dist = std::sqrt(distSq);

        glm::vec3 worldTriNormal = glm::normalize(glm::cross(v[1]-v[0], v[2]-v[0]));

        glm::vec3 normal;
        if (dist > 0.0001f) {
            normal = diff / dist;
        } else {
            normal = worldTriNormal;
        }
        
        // Ensure we always push out towards the triangle's front face
        if (glm::dot(normal, worldTriNormal) < 0.0f) {
            normal = -normal;
        }

        // Calculate world-space "radius" of the box along the normal axis.
        // We project the world axes of the box (which include scale) onto the normal.
        float worldRadius = 0.5f * (
            std::abs(glm::dot(normal, glm::vec3(tc.WorldTransform[0]))) * rbc.Size.x +
            std::abs(glm::dot(normal, glm::vec3(tc.WorldTransform[1]))) * rbc.Size.y +
            std::abs(glm::dot(normal, glm::vec3(tc.WorldTransform[2]))) * rbc.Size.z
        );

        if (dist < worldRadius)
        {
            float penetration = worldRadius - dist;
            if (penetration > maxPenetration)
            {
                maxPenetration = penetration;
                bestNormal = normal;
            }
        }
    }
    
    if (maxPenetration > 0.001f)
    {
        contacts.push_back({rbEntity, otherEntity, bestNormal, maxPenetration});
    }
}

void CollisionCore::ResolveCapsuleBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
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

    if (distSq >= seg.radius * seg.radius)
    {
        return;
    }

    float dist = std::sqrt(distSq);
    float penetration = seg.radius - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    contacts.push_back({rbEntity, otherEntity, normal, penetration});
}

void CollisionCore::ResolveCapsuleMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& capsule = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty())
    {
        return;
    }
    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh)
    {
        return;
    }

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    glm::mat4 invMeshMatrix = otherTc.InverseWorldTransform;

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
    if (candidates.empty())
    {
        return;
    }

    glm::vec3 bestNormal = {0.0f, 1.0f, 0.0f};
    float maxPenetration = -1.0f;


    for (const auto* tri : candidates)
    {
        glm::vec3 v0 = glm::vec3(meshMatrix * glm::vec4(tri->v0, 1.0f));
        glm::vec3 v1 = glm::vec3(meshMatrix * glm::vec4(tri->v1, 1.0f));
        glm::vec3 v2 = glm::vec3(meshMatrix * glm::vec4(tri->v2, 1.0f));

        glm::vec3 triPoint = ClosestPointTriangle(seg.a, v0, v1, v2);
        glm::vec3 segPoint = ClosestPointOnSegment(triPoint, seg.a, seg.b);
        triPoint = ClosestPointTriangle(segPoint, v0, v1, v2);
        glm::vec3 finalSeg = ClosestPointOnSegment(triPoint, seg.a, seg.b);

        glm::vec3 diff = finalSeg - triPoint;
        float distSq = glm::dot(diff, diff);
        float dist = std::sqrt(distSq);

        glm::vec3 worldTriNormal = glm::normalize(glm::cross(v1-v0, v2-v0));

        if (dist < seg.radius)
        {
            float penetration = seg.radius - dist;
            glm::vec3 normal = (dist > 0.001f) ? (diff / dist) : worldTriNormal;

            if (glm::dot(normal, worldTriNormal) < 0.0f)
                normal = -normal;

            if (penetration > maxPenetration)
            {
                maxPenetration = penetration;
                bestNormal = normal;
            }
        }
    }

    if (maxPenetration > 0.001f)
    {
        contacts.push_back({rbEntity, otherEntity, bestNormal, maxPenetration});
    }
}

void CollisionCore::ResolveSphereBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& box = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    WorldAABB boxAABB = GetWorldAABB(otherTc, box);
    glm::vec3 sphereWorldPos = glm::vec3(tc.WorldTransform * glm::vec4(sphere.Offset, 1.0f));

    glm::vec3 closestOnBox = glm::clamp(sphereWorldPos, boxAABB.Min, boxAABB.Max);

    glm::vec3 diff = sphereWorldPos - closestOnBox;
    float distSq = glm::dot(diff, diff);

    if (distSq >= sphere.Radius * sphere.Radius)
    {
        return;
    }

    float dist = std::sqrt(distSq);
    float penetration = sphere.Radius - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    contacts.push_back({rbEntity, otherEntity, normal, penetration});
}

void CollisionCore::ResolveSphereMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty())
    {
        return;
    }
    auto bvh = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvh)
    {
        return;
    }

    glm::mat4 meshMatrix = otherTc.WorldTransform;
    glm::mat4 invMeshMatrix = otherTc.InverseWorldTransform;

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
    if (candidates.empty())
    {
        return;
    }

    glm::vec3 bestNormal = {0.0f, 1.0f, 0.0f};
    float maxPenetration = -1.0f;


    for (const auto* tri : candidates)
    {
        glm::vec3 v0 = glm::vec3(meshMatrix * glm::vec4(tri->v0, 1.0f));
        glm::vec3 v1 = glm::vec3(meshMatrix * glm::vec4(tri->v1, 1.0f));
        glm::vec3 v2 = glm::vec3(meshMatrix * glm::vec4(tri->v2, 1.0f));

        glm::vec3 triPoint = ClosestPointTriangle(sphereWorldPos, v0, v1, v2);
        glm::vec3 diff = sphereWorldPos - triPoint;
        float dist = glm::length(diff);

        glm::vec3 worldTriNormal = glm::normalize(glm::cross(v1-v0, v2-v0));

        if (dist < sphere.Radius)
        {
            float penetration = sphere.Radius - dist;
            glm::vec3 normal = (dist > 0.001f) ? (diff / dist) : worldTriNormal;

            if (glm::dot(normal, worldTriNormal) < 0.0f)
                normal = -normal;

            if (penetration > maxPenetration)
            {
                maxPenetration = penetration;
                bestNormal = normal;
            }
        }
    }

    if (maxPenetration > 0.001f)
    {
        contacts.push_back({rbEntity, otherEntity, bestNormal, maxPenetration});
    }
}

void CollisionCore::ResolveSphereSphere(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc1 = registry.get<TransformComponent>(rbEntity);
    auto& s1  = registry.get<ColliderComponent>(rbEntity);
    auto& s2  = registry.get<ColliderComponent>(otherEntity);
    auto& tc2 = registry.get<TransformComponent>(otherEntity);

    glm::vec3 p1 = glm::vec3(tc1.WorldTransform * glm::vec4(s1.Offset, 1.0f));
    glm::vec3 p2 = glm::vec3(tc2.WorldTransform * glm::vec4(s2.Offset, 1.0f));

    glm::vec3 diff = p1 - p2;
    float distSq = glm::dot(diff, diff);
    float radiusSum = (s1.Radius * glm::length(glm::vec3(tc1.WorldTransform[0]))) +
                      (s2.Radius * glm::length(glm::vec3(tc2.WorldTransform[0])));

    if (distSq >= radiusSum * radiusSum)
    {
        return;
    }

    float dist = std::sqrt(distSq);
    float penetration = radiusSum - dist;
    glm::vec3 normal = (dist > 0.0001f) ? (diff / dist) : glm::vec3(0, 1, 0);

    contacts.push_back({rbEntity, otherEntity, normal, penetration});
}

void CollisionCore::ResolveMeshMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, std::vector<Contact>& contacts)
{
    auto& tc           = registry.get<TransformComponent>(rbEntity);
    auto& rbc          = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc      = registry.get<TransformComponent>(otherEntity);

    if (rbc.ModelPath.empty() || otherCollider.ModelPath.empty())
    {
        return;
    }

    auto bvhA = Physics::GetBVH(rbc.ModelPath);
    auto bvhB = Physics::GetBVH(otherCollider.ModelPath);
    if (!bvhA || !bvhB)
    {
        return;
    }

    // High-performance BVH-BVH recursive intersection
    glm::mat4 matAToB = otherTc.InverseWorldTransform * tc.WorldTransform;
    std::vector<BVH::BVHContact> bvhContacts;
    bvhA->IntersectBVH(*bvhB, matAToB, bvhContacts);

    // Apply responses for all detected contacts
    for (const auto& contact : bvhContacts)
    {
        // Transform the local normal from B space to World space
        glm::mat4 normalMatrix = glm::transpose(otherTc.InverseWorldTransform);
        glm::vec3 worldNormal = glm::normalize(glm::vec3(normalMatrix * glm::vec4(contact.worldNormal, 0.0f)));

        if (contact.depth > 0.001f)
        {
            contacts.push_back({rbEntity, otherEntity, worldNormal, contact.depth});
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Impulse-based Sequential Contact Solver
// ─────────────────────────────────────────────────────────────────────────────

void CollisionCore::SolveContacts(entt::registry& registry, std::vector<Contact>& contacts)
{
    for (auto& contact : contacts)
    {
        if (!registry.valid(contact.BodyA) || !registry.valid(contact.BodyB)) continue;

        auto* rbA   = registry.try_get<RigidBodyComponent>(contact.BodyA);
        auto* tcA   = registry.try_get<TransformComponent>(contact.BodyA);
        auto* colA  = registry.try_get<ColliderComponent>(contact.BodyA);
        if (!rbA || !tcA || !colA) continue;
        colA->IsColliding = true;

        const bool isDynamicB  = registry.all_of<RigidBodyComponent>(contact.BodyB);
        RigidBodyComponent* rbB  = nullptr;
        TransformComponent* tcB  = nullptr;

        if (isDynamicB)
        {
            rbB = registry.try_get<RigidBodyComponent>(contact.BodyB);
            tcB = registry.try_get<TransformComponent>(contact.BodyB);
            if (auto* colB = registry.try_get<ColliderComponent>(contact.BodyB)) colB->IsColliding = true;
        }

        // Inverse-mass: kinematic / static bodies are infinite mass (0 inv).
        const float invMassA  = rbA->IsKinematic ? 0.0f : (1.0f / rbA->Mass);
        const float invMassB  = (!isDynamicB || !rbB || rbB->IsKinematic) ? 0.0f : (1.0f / rbB->Mass);
        const float sumInvMass = invMassA + invMassB;
        if (sumInvMass <= 0.0f) continue;

        // ── Positional correction (Baumgarte with slop) ──
        const float kSlop    = 0.01f;
        const float kPercent = 0.8f;
        const float depth    = std::max(contact.Depth - kSlop, 0.0f);
        if (depth > 0.0f)
        {
            const glm::vec3 correction = contact.Normal * (depth * kPercent / sumInvMass);
            if (invMassA > 0.0f) { tcA->Translation += correction * invMassA; tcA->IsDirty = true; }
            if (invMassB > 0.0f) { tcB->Translation -= correction * invMassB; tcB->IsDirty = true; }
            contact.Depth -= depth * kPercent; // reduce for further iterations
        }

        // ── Normal impulse ──
        const glm::vec3 vA  = rbA->Velocity;
        const glm::vec3 vB  = (isDynamicB && rbB) ? rbB->Velocity : glm::vec3(0.0f);
        const glm::vec3 rv  = vA - vB;
        const float contactVel = glm::dot(rv, contact.Normal);
        if (contactVel > 0.0f) continue; // already separating

        const float e = 0.0f; // restitution = 0 (perfectly inelastic)
        const float j = -(1.0f + e) * contactVel / sumInvMass;
        const glm::vec3 impulse = contact.Normal * j;

        if (invMassA > 0.0f) rbA->Velocity += impulse * invMassA;
        if (invMassB > 0.0f) rbB->Velocity -= impulse * invMassB;

        // ── Friction impulse (Coulomb) ──
        const glm::vec3 rvAfter  = rbA->Velocity - ((isDynamicB && rbB) ? rbB->Velocity : glm::vec3(0.0f));
        glm::vec3 tangent        = rvAfter - contact.Normal * glm::dot(rvAfter, contact.Normal);
        const float tangentLen   = glm::length(tangent);
        if (tangentLen > 0.0001f)
        {
            tangent /= tangentLen;
            const float jt           = -glm::dot(rvAfter, tangent) / sumInvMass;
            const float mu           = 0.5f;
            const float maxFriction  = std::abs(j * mu);
            const float jClamped     = std::max(-maxFriction, std::min(jt, maxFriction));
            const glm::vec3 fImpulse = tangent * jClamped;

            if (invMassA > 0.0f) rbA->Velocity += fImpulse * invMassA;
            if (invMassB > 0.0f) rbB->Velocity -= fImpulse * invMassB;
        }

        // ── Grounded detection ──
        if (contact.Normal.y > 0.70f) rbA->IsGrounded = true;
        if (isDynamicB && rbB && contact.Normal.y < -0.70f) rbB->IsGrounded = true;

        // ── Notify gameplay ──
        if (auto* ctx = registry.ctx().find<PhysicsContext>())
            if (ctx->CollisionCallback)
                ctx->CollisionCallback(contact.BodyA, contact.BodyB);
    }
}


void CollisionCore::GenerateContacts(entt::registry& registry,
                                     const std::vector<entt::entity>& entities,
                                     std::vector<Contact>& contacts)
{
    auto allCollidersView = registry.view<TransformComponent, ColliderComponent>();
    
    // Query broadphase for each active rigid body
    for (auto rbEntity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent, ColliderComponent>(rbEntity))
            continue;

        auto& tc = registry.get<TransformComponent>(rbEntity);
        auto& rbCollider = registry.get<ColliderComponent>(rbEntity);
        WorldAABB rbAABB = GetWorldAABB(tc, rbCollider);
        
        for (auto otherEntity : allCollidersView)
        {
            if (rbEntity == otherEntity) continue;
            
            auto& otherTc = allCollidersView.get<TransformComponent>(otherEntity);
            auto& otherCollider = allCollidersView.get<ColliderComponent>(otherEntity);
            if (!otherCollider.Enabled) continue;

            // Double check AABB exactly
            WorldAABB targetAABB = GetWorldAABB(otherTc, otherCollider);
            if (!Collision::CheckAABB(rbAABB.Min - 0.1f, rbAABB.Max + 0.1f, targetAABB.Min, targetAABB.Max)) 
                continue;

            if (otherCollider.Type == ColliderType::Box)
            {
                if      (rbCollider.Type == ColliderType::Box)     ResolveBoxBox(registry, rbEntity, otherEntity, contacts);
                else if (rbCollider.Type == ColliderType::Capsule)  ResolveCapsuleBox(registry, rbEntity, otherEntity, contacts);
                else if (rbCollider.Type == ColliderType::Sphere)   ResolveSphereBox(registry, rbEntity, otherEntity, contacts);
            }
            else if (otherCollider.Type == ColliderType::Mesh)
            {
                if      (rbCollider.Type == ColliderType::Box)     ResolveBoxMesh(registry, rbEntity, otherEntity, contacts);
                else if (rbCollider.Type == ColliderType::Capsule)  ResolveCapsuleMesh(registry, rbEntity, otherEntity, contacts);
                else if (rbCollider.Type == ColliderType::Sphere)   ResolveSphereMesh(registry, rbEntity, otherEntity, contacts);
            }
            else if (otherCollider.Type == ColliderType::Sphere)
            {
                if (rbCollider.Type == ColliderType::Sphere) ResolveSphereSphere(registry, rbEntity, otherEntity, contacts);
            }
        }
    }
}

void CollisionCore::ResolveCollisions(entt::registry& registry, const std::vector<entt::entity>& entities)
{
    // Reset grounded state before this frame's solve.
    for (auto rbEntity : entities)
        if (auto* rb = registry.try_get<RigidBodyComponent>(rbEntity))
            rb->IsGrounded = false;

    // Collect all contacts in one broadphase + narrowphase pass.
    std::vector<Contact> contacts;
    contacts.reserve(64);
    GenerateContacts(registry, entities, contacts);

    // Iteratively solve — more iterations = more stable stacks.
    const int kSolverIterations = 8;
    for (int i = 0; i < kSolverIterations; ++i)
        SolveContacts(registry, contacts);

    // Propagate updated positions through the scene hierarchy.
    for (auto rbEntity : entities)
    {
        auto* tc = registry.try_get<TransformComponent>(rbEntity);
        if (!tc) continue;

        auto* hc = registry.try_get<HierarchyComponent>(rbEntity);
        if (!hc || hc->Parent == entt::null || !registry.valid(hc->Parent) || !registry.all_of<TransformComponent>(hc->Parent))
        {
            tc->WorldTransform = ComponentUtils::GetTransform(*tc);
        }
        else
        {
            tc->WorldTransform = registry.get<TransformComponent>(hc->Parent).WorldTransform * ComponentUtils::GetTransform(*tc);
        }
    }
}

} // namespace CHEngine
