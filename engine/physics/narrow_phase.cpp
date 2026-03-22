#include "narrow_phase.h"
#include "physics.h"
#include "bvh/bvh.h"
#include "cfloat"
#include "collision/collision.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "raymath.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"

namespace CHEngine
{

// ─── Helper: Apply collision response ───────
void NarrowPhase::ApplyResponse(::entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, TransformComponent& tc,
                                RigidBodyComponent& rb, ColliderComponent& other, Vector3 normal, float depth)
{
    // --- Baumgarte Stabilization & Slop ---
    // Instead of resolving 100% of the depth, we resolve a percentage (e.g. 40%) 
    // and ignore tiny penetrations (slop) to prevent jitter.
    const float kBaumgarte = 0.4f; 
    const float kSlop = 0.015f; 
    const float kMaxCorrection = 1.0f;

    float correctionDepth = fmaxf(depth - kSlop, 0.0f) * kBaumgarte;
    float correction = fminf(correctionDepth, kMaxCorrection);

    if (correction > 0.0f)
    {
        tc.Translation = Vector3Add(tc.Translation, Vector3Scale(normal, correction));
        tc.IsDirty = true;
        
        // Immediately update WorldTransform so subsequent collision checks in the same frame see the updated position
        auto hc = registry.try_get<HierarchyComponent>(rbEntity);
        if (!hc || hc->Parent == entt::null || !registry.valid(hc->Parent))
        {
            tc.WorldTransform = tc.GetTransform();
        }
        else
        {
            tc.WorldTransform = MatrixMultiply(tc.GetTransform(), registry.get<TransformComponent>(hc->Parent).WorldTransform);
        }
    }

    // Grounding
    if (normal.y > 0.45f)
    {
        rb.IsGrounded = true;

        // if (correction > 0.1f)
        // {
        //     std::string nameA = "Unknown";
        //     std::string nameB = "Unknown";
        //     if (registry.all_of<TagComponent>(rbEntity)) nameA = registry.get<TagComponent>(rbEntity).Tag;
        //     if (registry.all_of<TagComponent>(otherEntity)) nameB = registry.get<TagComponent>(otherEntity).Tag;

        //     CH_CORE_WARN("Physics: Upward correction ({:.3f}) for [{}] hitting [{}]", 
        //                  correction, nameA, nameB);
        // }

        // If we hit ground and were falling or moving up slightly (drift), stop vertical movement
        if (rb.Velocity.y < 0.1f)
        {
            rb.Velocity.y = 0;
        }

        // Stability: zero out horizontal velocity if it's very small after collision
        if (fabsf(rb.Velocity.x) < 0.05f) rb.Velocity.x = 0;
        if (fabsf(rb.Velocity.z) < 0.05f) rb.Velocity.z = 0;
    }
    else if (normal.y < -0.45f) // Ceiling
    {
        if (rb.Velocity.y > 0)
        {
            rb.Velocity.y = 0;
        }
    }

    // Velocity cancellation: remove component pointing into the surface
    float vDotN = Vector3DotProduct(rb.Velocity, normal);
    if (vDotN < 0.0f)
    {
        rb.Velocity = Vector3Subtract(rb.Velocity, Vector3Scale(normal, vDotN));
    }

    other.IsColliding = true;

    // Trigger callback
    if (auto* context = registry.ctx().find<PhysicsContext>())
    {
        if (context->CollisionCallback)
        {
            context->CollisionCallback(rbEntity, otherEntity);
        }
    }
}

Vector3 NarrowPhase::ClosestPointOnSegment(Vector3 p, Vector3 a, Vector3 b)
{
    Vector3 ab = Vector3Subtract(b, a);
    float denom = Vector3DotProduct(ab, ab);
    if (denom < 0.0001f)
    {
        return a;
    }
    float t = fmaxf(0.0f, fminf(1.0f, Vector3DotProduct(Vector3Subtract(p, a), ab) / denom));
    return Vector3Add(a, Vector3Scale(ab, t));
}

Vector3 NarrowPhase::ClosestPointTriangle(Vector3 p, Vector3 a, Vector3 b, Vector3 c)
{
    Vector3 ab = Vector3Subtract(b, a);
    Vector3 ac = Vector3Subtract(c, a);
    Vector3 ap = Vector3Subtract(p, a);
    float d1 = Vector3DotProduct(ab, ap);
    float d2 = Vector3DotProduct(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        return a;
    }

    Vector3 bp = Vector3Subtract(p, b);
    float d3 = Vector3DotProduct(ab, bp);
    float d4 = Vector3DotProduct(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        return b;
    }

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return Vector3Add(a, Vector3Scale(ab, v));
    }

    Vector3 cp = Vector3Subtract(p, c);
    float d5 = Vector3DotProduct(ab, cp);
    float d6 = Vector3DotProduct(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        return c;
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return Vector3Add(a, Vector3Scale(ac, w));
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return Vector3Add(b, Vector3Scale(Vector3Subtract(c, b), w));
    }

    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return Vector3Add(a, Vector3Add(Vector3Scale(ab, v), Vector3Scale(ac, w)));
}

NarrowPhase::CapsuleSegment NarrowPhase::GetCapsuleSegment(const TransformComponent& tc, const ColliderComponent& cc)
{
    // Extract properties from WorldTransform
    Vector3 worldPos = { tc.WorldTransform.m12, tc.WorldTransform.m13, tc.WorldTransform.m14 };
    
    // Offset is in local space, so it must be rotated and scaled by WorldTransform
    // However, for simplicity and performance in most games, we treat Offset as being in Entity space.
    // To be perfectly accurate in a hierarchy:
    Vector3 worldOffset = Vector3Subtract(Vector3Transform(cc.Offset, tc.WorldTransform), worldPos);
    Vector3 pos = Vector3Add(worldPos, worldOffset);
    
    // For now, our capsule is always vertically aligned in world or local space?
    // Usually capsules are character-aligned (vertical).
    float halfSeg = fmaxf(0.0f, cc.Height * 0.5f - cc.Radius);
    
    // Apply world scale to height/radius if needed? 
    // Usually character capsules are not scaled much, but for robustness:
    float worldScaleY = Vector3Length({tc.WorldTransform.m4, tc.WorldTransform.m5, tc.WorldTransform.m6});
    float worldScaleXZ = fmaxf(Vector3Length({tc.WorldTransform.m0, tc.WorldTransform.m1, tc.WorldTransform.m2}), 
                               Vector3Length({tc.WorldTransform.m8, tc.WorldTransform.m9, tc.WorldTransform.m10}));
                               
    float finalRadius = cc.Radius * worldScaleXZ;
    float finalHalfSeg = halfSeg * worldScaleY;

    return {{pos.x, pos.y - finalHalfSeg, pos.z}, {pos.x, pos.y + finalHalfSeg, pos.z}, finalRadius};
}

NarrowPhase::WorldAABB NarrowPhase::GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc)
{
    // AABB must perfectly enclose the rotated/scaled box in world space.
    // For a Box collider, cc.Offset is the minimum point and cc.Size is the extent.
    Vector3 minLocal = cc.Offset;
    Vector3 maxLocal = Vector3Add(cc.Offset, cc.Size);

    Vector3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}
    };

    Vector3 worldMin = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 worldMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int i = 0; i < 8; i++)
    {
        Vector3 worldPt = Vector3Transform(corners[i], tc.WorldTransform);
        worldMin = Vector3Min(worldMin, worldPt);
        worldMax = Vector3Max(worldMax, worldPt);
    }

    return {worldMin, worldMax};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main dispatch
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveCollisions(::entt::registry& registry, const std::vector<::entt::entity>& entities)
{
    for (auto rbEntity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent, ColliderComponent>(rbEntity))
        {
            continue;
        }

        auto& rb = registry.get<RigidBodyComponent>(rbEntity);
        rb.IsGrounded = false;

        auto& rbCollider = registry.get<ColliderComponent>(rbEntity);

        auto colliders = registry.view<TransformComponent, ColliderComponent>();
        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity)
            {
                continue;
            }

            auto& otherCollider = colliders.get<ColliderComponent>(otherEntity);
            if (!otherCollider.Enabled)
            {
                continue;
            }

            if (otherCollider.Type == ColliderType::Box)
            {
                if (rbCollider.Type == ColliderType::Box)
                {
                    ResolveBoxBox(registry, rbEntity, otherEntity);
                }
                else if (rbCollider.Type == ColliderType::Capsule)
                {
                    ResolveCapsuleBox(registry, rbEntity, otherEntity);
                }
                else if (rbCollider.Type == ColliderType::Sphere)
                {
                    ResolveSphereBox(registry, rbEntity, otherEntity);
                }
            }
            else if (otherCollider.Type == ColliderType::Mesh)
            {
                if (rbCollider.Type == ColliderType::Box)
                {
                    ResolveBoxMesh(registry, rbEntity, otherEntity);
                }
                else if (rbCollider.Type == ColliderType::Capsule)
                {
                    ResolveCapsuleMesh(registry, rbEntity, otherEntity);
                }
                else if (rbCollider.Type == ColliderType::Sphere)
                {
                    ResolveSphereMesh(registry, rbEntity, otherEntity);
                }
            }
            else if (otherCollider.Type == ColliderType::Sphere)
            {
                if (rbCollider.Type == ColliderType::Sphere)
                {
                    ResolveSphereSphere(registry, rbEntity, otherEntity);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Box vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveBoxBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);

    NarrowPhase::WorldAABB a = NarrowPhase::GetWorldAABB(tc, rbc);
    NarrowPhase::WorldAABB b = NarrowPhase::GetWorldAABB(registry.get<TransformComponent>(otherEntity), otherCollider);

    if (!Collision::CheckAABB(a.min, a.max, b.min, b.max))
    {
        return;
    }

    // Find minimum penetration axis
    float depths[6] = {b.max.x - a.min.x, a.max.x - b.min.x, b.max.y - a.min.y,
                       a.max.y - b.min.y, b.max.z - a.min.z, a.max.z - b.min.z};

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

    // MTV direction per axis: +X, -X, +Y, -Y, +Z, -Z
    const Vector3 dirs[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, dirs[axis], minDepth);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Box vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveBoxMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& rbc = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    auto project = Project::GetActive();
    if (!project) return;

    auto bvh = PhysicsSystem::Get().GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    // Transform to mesh local space
    Matrix meshMatrix = otherTc.WorldTransform;
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);
    Matrix localToOtherLocal = MatrixMultiply(tc.WorldTransform, invMeshMatrix);

    Vector3 minLocal = rbc.Offset;
    Vector3 maxLocal = Vector3Add(rbc.Offset, rbc.Size);
    Vector3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}
    };

    BoundingBox localBox = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    for (int k = 0; k < 8; k++)
    {
        Vector3 lc = Vector3Transform(corners[k], localToOtherLocal);
        localBox.min = Vector3Min(localBox.min, lc);
        localBox.max = Vector3Max(localBox.max, lc);
    }


    Vector3 localNormal;
    float overlapDepth = -1.0f;
    if (!bvh->IntersectAABB(localBox, localNormal, overlapDepth))
    {
        return;
    }

    if (overlapDepth <= 0.0001f)
    {
        return;
    }

    // Transform MTV back to world space
    Vector3 origin = Vector3Transform({0, 0, 0}, meshMatrix);
    Vector3 worldMTV = Vector3Subtract(Vector3Transform(Vector3Scale(localNormal, overlapDepth), meshMatrix), origin);

    Matrix normalMatrix = MatrixTranspose(invMeshMatrix);
    Vector3 worldNormal = Vector3Normalize(
        Vector3Subtract(Vector3Transform(localNormal, normalMatrix), Vector3Transform({0, 0, 0}, normalMatrix)));

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, otherCollider, worldNormal, Vector3Length(worldMTV));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Capsule vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveCapsuleBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& capsule = registry.get<ColliderComponent>(rbEntity);
    auto& box = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    NarrowPhase::CapsuleSegment seg = NarrowPhase::GetCapsuleSegment(tc, capsule);
    NarrowPhase::WorldAABB boxAABB = NarrowPhase::GetWorldAABB(otherTc, box);

    // Find closest point on box to closest point on capsule segment
    Vector3 boxCenter = Vector3Scale(Vector3Add(boxAABB.min, boxAABB.max), 0.5f);
    Vector3 closestOnSeg = NarrowPhase::ClosestPointOnSegment(boxCenter, seg.a, seg.b);

    // Clamp to box surface
    Vector3 closestOnBox = {fmaxf(boxAABB.min.x, fminf(closestOnSeg.x, boxAABB.max.x)),
                            fmaxf(boxAABB.min.y, fminf(closestOnSeg.y, boxAABB.max.y)),
                            fmaxf(boxAABB.min.z, fminf(closestOnSeg.z, boxAABB.max.z))};

    // Re-project: find closest point on segment to the box surface point
    Vector3 finalOnSeg = NarrowPhase::ClosestPointOnSegment(closestOnBox, seg.a, seg.b);
    Vector3 diff = Vector3Subtract(finalOnSeg, closestOnBox);
    float distSq = Vector3DotProduct(diff, diff);

    if (distSq >= seg.radius * seg.radius)
    {
        return;
    }

    float dist = sqrtf(distSq);
    float penetration = seg.radius - dist;

    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, box, normal, penetration);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Capsule vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
//  Capsule vs Mesh
// ─────────────────────────────────────────────────────────────────────────────

void NarrowPhase::ResolveCapsuleMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& capsule = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    auto project = Project::GetActive();
    if (!project) return;

    auto bvh = PhysicsSystem::Get().GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    Matrix meshMatrix = otherTc.WorldTransform;
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    CapsuleSegment seg = GetCapsuleSegment(tc, capsule);

    // Build query AABB in mesh local space
    Vector3 localA = Vector3Transform(seg.a, invMeshMatrix);
    Vector3 localB = Vector3Transform(seg.b, invMeshMatrix);

    // localRadius should scale with local-to-world? 
    // Actually, localRadius is in the mesh local space.
    // If meshMatrix has scale, say 2.0, then a world radius of 1.0 becomes 0.5 in local space.
    float scaleX = Vector3Length({meshMatrix.m0, meshMatrix.m1, meshMatrix.m2});
    float scaleY = Vector3Length({meshMatrix.m4, meshMatrix.m5, meshMatrix.m6});
    float scaleZ = Vector3Length({meshMatrix.m8, meshMatrix.m9, meshMatrix.m10});

    Vector3 localExtents = {
        (scaleX > 0.0001f) ? seg.radius / scaleX : seg.radius,
        (scaleY > 0.0001f) ? seg.radius / scaleY : seg.radius,
        (scaleZ > 0.0001f) ? seg.radius / scaleZ : seg.radius
    };

    Vector3 minSeg = Vector3Min(localA, localB);
    Vector3 maxSeg = Vector3Max(localA, localB);
    BoundingBox queryBox = {{minSeg.x - localExtents.x, minSeg.y - localExtents.y, minSeg.z - localExtents.z},
                            {maxSeg.x + localExtents.x, maxSeg.y + localExtents.y, maxSeg.z + localExtents.z}};

    // Query BVH for candidate triangles
    std::vector<const CollisionTriangle*> candidates;
    bvh->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    // Find deepest penetration among all candidate triangles
    Vector3 bestNormal      = {0.0f, 1.0f, 0.0f};
    float   maxPenetration  = -1.0f;
    bool    anyContact      = false;

    for (const auto* tri : candidates)
    {
        // Transform triangle to world space
        Vector3 v0 = Vector3Transform(tri->v0, meshMatrix);
        Vector3 v1 = Vector3Transform(tri->v1, meshMatrix);
        Vector3 v2 = Vector3Transform(tri->v2, meshMatrix);

        // Find closest point on triangle, then closest on capsule segment
        Vector3 triCenter = Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
        Vector3 segPoint = NarrowPhase::ClosestPointOnSegment(triCenter, seg.a, seg.b);
        Vector3 triPoint = NarrowPhase::ClosestPointTriangle(segPoint, v0, v1, v2);

        // Re-project back to segment for accuracy
        Vector3 finalSeg = NarrowPhase::ClosestPointOnSegment(triPoint, seg.a, seg.b);
        Vector3 diff = Vector3Subtract(finalSeg, triPoint);
        float distSq = Vector3DotProduct(diff, diff);

        if (distSq >= seg.radius * seg.radius) continue;

        float dist = sqrtf(distSq);
        float penetration = seg.radius - dist;
        Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : tri->normal;

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

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& box = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    NarrowPhase::WorldAABB boxAABB = NarrowPhase::GetWorldAABB(otherTc, box);
    
    Vector3 sphereWorldPos = Vector3Add({tc.WorldTransform.m12, tc.WorldTransform.m13, tc.WorldTransform.m14}, 
                                       Vector3Subtract(Vector3Transform(sphere.Offset, tc.WorldTransform), 
                                                       {tc.WorldTransform.m12, tc.WorldTransform.m13, tc.WorldTransform.m14}));

    Vector3 closestOnBox = {fmaxf(boxAABB.min.x, fminf(sphereWorldPos.x, boxAABB.max.x)),
                            fmaxf(boxAABB.min.y, fminf(sphereWorldPos.y, boxAABB.max.y)),
                            fmaxf(boxAABB.min.z, fminf(sphereWorldPos.z, boxAABB.max.z))};

    Vector3 diff = Vector3Subtract(sphereWorldPos, closestOnBox);
    float distSq = Vector3DotProduct(diff, diff);

    if (distSq >= sphere.Radius * sphere.Radius)
    {
        return;
    }

    float dist = sqrtf(distSq);
    float penetration = sphere.Radius - dist;
    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, box, normal, penetration);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& sphere = registry.get<ColliderComponent>(rbEntity);
    auto& otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto& otherTc = registry.get<TransformComponent>(otherEntity);

    if (otherCollider.ModelPath.empty()) return;
    auto project = Project::GetActive();
    if (!project) return;

    auto bvh = PhysicsSystem::Get().GetBVH(otherCollider.ModelPath);
    if (!bvh) return;

    Matrix meshMatrix = otherTc.WorldTransform;
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    Vector3 sphereWorldPos = Vector3Add({tc.WorldTransform.m12, tc.WorldTransform.m13, tc.WorldTransform.m14}, 
                                       Vector3Subtract(Vector3Transform(sphere.Offset, tc.WorldTransform), 
                                                       {tc.WorldTransform.m12, tc.WorldTransform.m13, tc.WorldTransform.m14}));
    Vector3 sphereLocalPos = Vector3Transform(sphereWorldPos, invMeshMatrix);

    float scaleX = Vector3Length({meshMatrix.m0, meshMatrix.m1, meshMatrix.m2});
    float scaleY = Vector3Length({meshMatrix.m4, meshMatrix.m5, meshMatrix.m6});
    float scaleZ = Vector3Length({meshMatrix.m8, meshMatrix.m9, meshMatrix.m10});

    Vector3 localExtents = {
        (scaleX > 0.0001f) ? sphere.Radius / scaleX : sphere.Radius,
        (scaleY > 0.0001f) ? sphere.Radius / scaleY : sphere.Radius,
        (scaleZ > 0.0001f) ? sphere.Radius / scaleZ : sphere.Radius
    };

    BoundingBox queryBox = {
        {sphereLocalPos.x - localExtents.x, sphereLocalPos.y - localExtents.y, sphereLocalPos.z - localExtents.z},
        {sphereLocalPos.x + localExtents.x, sphereLocalPos.y + localExtents.y, sphereLocalPos.z + localExtents.z}};

    std::vector<const CollisionTriangle*> candidates;
    bvh->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    // Find deepest penetration
    Vector3 bestNormal      = {0.0f, 1.0f, 0.0f};
    float   maxPenetration  = -1.0f;
    bool    anyContact      = false;

    for (const auto* tri : candidates)
    {
        Vector3 v0 = Vector3Transform(tri->v0, meshMatrix);
        Vector3 v1 = Vector3Transform(tri->v1, meshMatrix);
        Vector3 v2 = Vector3Transform(tri->v2, meshMatrix);

        Vector3 triPoint = NarrowPhase::ClosestPointTriangle(sphereWorldPos, v0, v1, v2);
        Vector3 diff = Vector3Subtract(sphereWorldPos, triPoint);
        float distSq = Vector3DotProduct(diff, diff);

        if (distSq >= sphere.Radius * sphere.Radius) continue;

        float dist = sqrtf(distSq);
        float penetration = sphere.Radius - dist;
        Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : tri->normal;

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

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Sphere
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereSphere(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity)
{
    auto& tc = registry.get<TransformComponent>(rbEntity);
    auto& rb = registry.get<RigidBodyComponent>(rbEntity);
    auto& s1 = registry.get<ColliderComponent>(rbEntity);
    auto& s2 = registry.get<ColliderComponent>(otherEntity);
    auto& tc2 = registry.get<TransformComponent>(otherEntity);

    Vector3 p1 = Vector3Add(tc.Translation, s1.Offset);
    Vector3 p2 = Vector3Add(tc2.Translation, s2.Offset);

    Vector3 diff = Vector3Subtract(p1, p2);
    float distSq = Vector3DotProduct(diff, diff);
    float radiusSum = s1.Radius + s2.Radius;

    if (distSq >= radiusSum * radiusSum)
    {
        return;
    }

    float dist = sqrtf(distSq);
    float penetration = radiusSum - dist;
    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(registry, rbEntity, otherEntity, tc, rb, s2, normal, penetration);
}

} // namespace CHEngine