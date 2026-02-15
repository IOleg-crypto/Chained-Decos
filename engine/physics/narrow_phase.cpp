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

// ─── Helper: Apply collision response ────────────────────────────────────────
static void ApplyResponse(TransformComponent &tc, RigidBodyComponent &rb, ColliderComponent &other,
                          Vector3 normal, float depth)
{
    tc.Translation = Vector3Add(tc.Translation, Vector3Scale(normal, depth));

    // Grounding
    if (normal.y > 0.45f)
    {
        rb.IsGrounded = true;
        if (rb.Velocity.y < 0) rb.Velocity.y = 0;
    }
    else if (normal.y < -0.5f)
    {
        if (rb.Velocity.y > 0) rb.Velocity.y = 0;
    }

    // Slide along surface
    float dot = Vector3DotProduct(rb.Velocity, normal);
    if (dot < 0.0f)
        rb.Velocity = Vector3Subtract(rb.Velocity, Vector3Scale(normal, dot));

    other.IsColliding = true;
}

// ─── Helper: Closest point on line segment ───────────────────────────────────
static Vector3 ClosestPointOnSegment(Vector3 p, Vector3 a, Vector3 b)
{
    Vector3 ab = Vector3Subtract(b, a);
    float denom = Vector3DotProduct(ab, ab);
    if (denom < 0.0001f) return a;
    float t = fmaxf(0.0f, fminf(1.0f, Vector3DotProduct(Vector3Subtract(p, a), ab) / denom));
    return Vector3Add(a, Vector3Scale(ab, t));
}

// ─── Helper: Closest point on triangle (Voronoi regions) ─────────────────────
static Vector3 ClosestPointTriangle(Vector3 p, Vector3 a, Vector3 b, Vector3 c)
{
    Vector3 ab = Vector3Subtract(b, a);
    Vector3 ac = Vector3Subtract(c, a);
    Vector3 ap = Vector3Subtract(p, a);
    float d1 = Vector3DotProduct(ab, ap);
    float d2 = Vector3DotProduct(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    Vector3 bp = Vector3Subtract(p, b);
    float d3 = Vector3DotProduct(ab, bp);
    float d4 = Vector3DotProduct(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return Vector3Add(a, Vector3Scale(ab, v));
    }

    Vector3 cp = Vector3Subtract(p, c);
    float d5 = Vector3DotProduct(ab, cp);
    float d6 = Vector3DotProduct(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

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

// ─── Helper: Get capsule segment endpoints ───────────────────────────────────
struct CapsuleSegment { Vector3 a, b; float radius; };

static CapsuleSegment GetCapsuleSegment(const TransformComponent &tc, const ColliderComponent &cc)
{
    Vector3 pos = Vector3Add(tc.Translation, cc.Offset);
    float halfSeg = fmaxf(0.0f, cc.Height * 0.5f - cc.Radius);
    return {
        {pos.x, pos.y - halfSeg, pos.z},
        {pos.x, pos.y + halfSeg, pos.z},
        cc.Radius
    };
}

// ─── Helper: Get world AABB from collider ────────────────────────────────────
struct WorldAABB { Vector3 min, max; };

static WorldAABB GetWorldAABB(const TransformComponent &tc, const ColliderComponent &cc)
{
    Vector3 scale = tc.Scale;
    Vector3 offset = Vector3Multiply(cc.Offset, scale);
    Vector3 size = Vector3Multiply(cc.Size, scale);
    Vector3 min = Vector3Add(tc.Translation, offset);
    return {min, Vector3Add(min, size)};
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main dispatch
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveCollisions(Scene *scene, const std::vector<::entt::entity> &entities)
{
    auto &registry = scene->GetRegistry();

    for (auto rbEntity : entities)
    {
        if (!registry.all_of<TransformComponent, RigidBodyComponent, ColliderComponent>(rbEntity))
            continue;

        auto &rb = registry.get<RigidBodyComponent>(rbEntity);
        rb.IsGrounded = false;

        auto &rbCollider = registry.get<ColliderComponent>(rbEntity);

        auto colliders = registry.view<TransformComponent, ColliderComponent>();
        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity) continue;

            auto &otherCollider = colliders.get<ColliderComponent>(otherEntity);
            if (!otherCollider.Enabled) continue;

            if (otherCollider.Type == ColliderType::Box)
            {
                if (rbCollider.Type == ColliderType::Box)
                    ResolveBoxBox(registry, rbEntity, otherEntity);
                else if (rbCollider.Type == ColliderType::Capsule)
                    ResolveCapsuleBox(registry, rbEntity, otherEntity);
                else if (rbCollider.Type == ColliderType::Sphere)
                    ResolveSphereBox(registry, rbEntity, otherEntity);
            }
            else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
            {
                if (rbCollider.Type == ColliderType::Box)
                    ResolveBoxMesh(registry, rbEntity, otherEntity);
                else if (rbCollider.Type == ColliderType::Capsule)
                    ResolveCapsuleMesh(registry, rbEntity, otherEntity);
                else if (rbCollider.Type == ColliderType::Sphere)
                    ResolveSphereMesh(registry, rbEntity, otherEntity);
            }
            else if (otherCollider.Type == ColliderType::Sphere)
            {
                 if (rbCollider.Type == ColliderType::Sphere)
                    ResolveSphereSphere(registry, rbEntity, otherEntity);
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Box vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveBoxBox(::entt::registry &registry, ::entt::entity rbEntity,
                                ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &rbc = registry.get<ColliderComponent>(rbEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);

    WorldAABB a = GetWorldAABB(tc, rbc);
    WorldAABB b = GetWorldAABB(registry.get<TransformComponent>(otherEntity), otherCollider);

    if (!Collision::CheckAABB(a.min, a.max, b.min, b.max))
        return;

    // Find minimum penetration axis
    float depths[6] = {
        b.max.x - a.min.x, a.max.x - b.min.x,
        b.max.y - a.min.y, a.max.y - b.min.y,
        b.max.z - a.min.z, a.max.z - b.min.z
    };

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

    // MTV direction per axis: +X, -X, +Y, -Y, +Z, -Z
    const Vector3 dirs[6] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    ApplyResponse(tc, rb, otherCollider, dirs[axis], minDepth);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Box vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveBoxMesh(entt::registry &registry, entt::entity rbEntity,
                                 entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &rbc = registry.get<ColliderComponent>(rbEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto &otherTc = registry.get<TransformComponent>(otherEntity);

    // Build world-space AABB for the rigid body
    WorldAABB rbAABB = GetWorldAABB(tc, rbc);
    BoundingBox rbBox = {rbAABB.min, rbAABB.max};

    // Transform to mesh local space
    Matrix meshMatrix = otherTc.GetTransform();
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    Vector3 corners[8] = {
        {rbBox.min.x, rbBox.min.y, rbBox.min.z}, {rbBox.max.x, rbBox.min.y, rbBox.min.z},
        {rbBox.min.x, rbBox.max.y, rbBox.min.z}, {rbBox.max.x, rbBox.max.y, rbBox.min.z},
        {rbBox.min.x, rbBox.min.y, rbBox.max.z}, {rbBox.max.x, rbBox.min.y, rbBox.max.z},
        {rbBox.min.x, rbBox.max.y, rbBox.max.z}, {rbBox.max.x, rbBox.max.y, rbBox.max.z}
    };

    BoundingBox localBox = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}};
    for (int k = 0; k < 8; k++)
    {
        Vector3 lc = Vector3Transform(corners[k], invMeshMatrix);
        localBox.min = Vector3Min(localBox.min, lc);
        localBox.max = Vector3Max(localBox.max, lc);
    }

    Vector3 localNormal;
    float overlapDepth = -1.0f;
    if (!otherCollider.BVHRoot->IntersectAABB(localBox, localNormal, overlapDepth))
        return;

    if (overlapDepth <= 0.0001f) return;

    // Transform MTV back to world space
    Vector3 origin = Vector3Transform({0, 0, 0}, meshMatrix);
    Vector3 worldMTV = Vector3Subtract(
        Vector3Transform(Vector3Scale(localNormal, overlapDepth), meshMatrix), origin);

    Matrix normalMatrix = MatrixTranspose(invMeshMatrix);
    Vector3 worldNormal = Vector3Normalize(
        Vector3Subtract(Vector3Transform(localNormal, normalMatrix),
                        Vector3Transform({0, 0, 0}, normalMatrix)));

    ApplyResponse(tc, rb, otherCollider, worldNormal, Vector3Length(worldMTV));
}

// ═══════════════════════════════════════════════════════════════════════════════
// Capsule vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveCapsuleBox(::entt::registry &registry, ::entt::entity rbEntity,
                                    ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &capsule = registry.get<ColliderComponent>(rbEntity);
    auto &box = registry.get<ColliderComponent>(otherEntity);
    auto &otherTc = registry.get<TransformComponent>(otherEntity);

    CapsuleSegment seg = GetCapsuleSegment(tc, capsule);
    WorldAABB boxAABB = GetWorldAABB(otherTc, box);

    // Find closest point on box to closest point on capsule segment
    Vector3 boxCenter = Vector3Scale(Vector3Add(boxAABB.min, boxAABB.max), 0.5f);
    Vector3 closestOnSeg = ClosestPointOnSegment(boxCenter, seg.a, seg.b);

    // Clamp to box surface
    Vector3 closestOnBox = {
        fmaxf(boxAABB.min.x, fminf(closestOnSeg.x, boxAABB.max.x)),
        fmaxf(boxAABB.min.y, fminf(closestOnSeg.y, boxAABB.max.y)),
        fmaxf(boxAABB.min.z, fminf(closestOnSeg.z, boxAABB.max.z))
    };

    // Re-project: find closest point on segment to the box surface point
    Vector3 finalOnSeg = ClosestPointOnSegment(closestOnBox, seg.a, seg.b);
    Vector3 diff = Vector3Subtract(finalOnSeg, closestOnBox);
    float distSq = Vector3DotProduct(diff, diff);

    if (distSq >= seg.radius * seg.radius) return;

    float dist = sqrtf(distSq);
    float penetration = seg.radius - dist;

    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(tc, rb, box, normal, penetration);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Capsule vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveCapsuleMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                     ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &capsule = registry.get<ColliderComponent>(rbEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto &otherTc = registry.get<TransformComponent>(otherEntity);

    Matrix meshMatrix = otherTc.GetTransform();
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    CapsuleSegment seg = GetCapsuleSegment(tc, capsule);

    // Build query AABB in mesh local space
    Vector3 localA = Vector3Transform(seg.a, invMeshMatrix);
    Vector3 localB = Vector3Transform(seg.b, invMeshMatrix);

    float maxScale = fmaxf(otherTc.Scale.x, fmaxf(otherTc.Scale.y, otherTc.Scale.z));
    float localRadius = (maxScale > 0.0001f) ? seg.radius / maxScale : seg.radius;

    Vector3 minSeg = Vector3Min(localA, localB);
    Vector3 maxSeg = Vector3Max(localA, localB);
    BoundingBox queryBox = {
        {minSeg.x - localRadius, minSeg.y - localRadius, minSeg.z - localRadius},
        {maxSeg.x + localRadius, maxSeg.y + localRadius, maxSeg.z + localRadius}
    };

    // Query BVH for candidate triangles
    std::vector<const CollisionTriangle *> candidates;
    otherCollider.BVHRoot->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    for (const auto *tri : candidates)
    {
        // Transform triangle to world space
        Vector3 v0 = Vector3Transform(tri->v0, meshMatrix);
        Vector3 v1 = Vector3Transform(tri->v1, meshMatrix);
        Vector3 v2 = Vector3Transform(tri->v2, meshMatrix);

        // Find closest point on triangle, then closest on capsule segment
        Vector3 triCenter = Vector3Scale(Vector3Add(Vector3Add(v0, v1), v2), 1.0f / 3.0f);
        Vector3 segPoint = ClosestPointOnSegment(triCenter, seg.a, seg.b);
        Vector3 triPoint = ClosestPointTriangle(segPoint, v0, v1, v2);

        // Re-project back to segment for accuracy
        Vector3 finalSeg = ClosestPointOnSegment(triPoint, seg.a, seg.b);
        Vector3 diff = Vector3Subtract(finalSeg, triPoint);
        float distSq = Vector3DotProduct(diff, diff);

        if (distSq >= seg.radius * seg.radius) continue;

        float dist = sqrtf(distSq);
        float penetration = seg.radius - dist;

        Vector3 normal;
        if (dist > 0.0001f)
            normal = Vector3Scale(diff, 1.0f / dist);
        else
            normal = Vector3Normalize(Vector3CrossProduct(
                Vector3Subtract(v1, v0), Vector3Subtract(v2, v0)));

        ApplyResponse(tc, rb, otherCollider, normal, penetration);

        // Update capsule position for stacking contacts
        seg = GetCapsuleSegment(tc, capsule);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Box
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereBox(::entt::registry &registry, ::entt::entity rbEntity,
                                   ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &sphere = registry.get<ColliderComponent>(rbEntity);
    auto &box = registry.get<ColliderComponent>(otherEntity);
    auto &otherTc = registry.get<TransformComponent>(otherEntity);

    WorldAABB boxAABB = GetWorldAABB(otherTc, box);
    Vector3 spherePos = Vector3Add(tc.Translation, sphere.Offset);

    Vector3 closestOnBox = {
        fmaxf(boxAABB.min.x, fminf(spherePos.x, boxAABB.max.x)),
        fmaxf(boxAABB.min.y, fminf(spherePos.y, boxAABB.max.y)),
        fmaxf(boxAABB.min.z, fminf(spherePos.z, boxAABB.max.z))
    };

    Vector3 diff = Vector3Subtract(spherePos, closestOnBox);
    float distSq = Vector3DotProduct(diff, diff);

    if (distSq >= sphere.Radius * sphere.Radius) return;

    float dist = sqrtf(distSq);
    float penetration = sphere.Radius - dist;
    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(tc, rb, box, normal, penetration);
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Mesh
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                    ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &sphere = registry.get<ColliderComponent>(rbEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);
    auto &otherTc = registry.get<TransformComponent>(otherEntity);

    Matrix meshMatrix = otherTc.GetTransform();
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    Vector3 sphereWorldPos = Vector3Add(tc.Translation, sphere.Offset);
    Vector3 sphereLocalPos = Vector3Transform(sphereWorldPos, invMeshMatrix);

    float maxScale = fmaxf(otherTc.Scale.x, fmaxf(otherTc.Scale.y, otherTc.Scale.z));
    float localRadius = (maxScale > 0.0001f) ? sphere.Radius / maxScale : sphere.Radius;

    BoundingBox queryBox = {
        {sphereLocalPos.x - localRadius, sphereLocalPos.y - localRadius, sphereLocalPos.z - localRadius},
        {sphereLocalPos.x + localRadius, sphereLocalPos.y + localRadius, sphereLocalPos.z + localRadius}
    };

    std::vector<const CollisionTriangle *> candidates;
    otherCollider.BVHRoot->QueryAABB(queryBox, candidates);
    if (candidates.empty()) return;

    for (const auto *tri : candidates)
    {
        Vector3 v0 = Vector3Transform(tri->v0, meshMatrix);
        Vector3 v1 = Vector3Transform(tri->v1, meshMatrix);
        Vector3 v2 = Vector3Transform(tri->v2, meshMatrix);

        Vector3 triPoint = ClosestPointTriangle(sphereWorldPos, v0, v1, v2);
        Vector3 diff = Vector3Subtract(sphereWorldPos, triPoint);
        float distSq = Vector3DotProduct(diff, diff);

        if (distSq >= sphere.Radius * sphere.Radius) continue;

        float dist = sqrtf(distSq);
        float penetration = sphere.Radius - dist;
        Vector3 normal;
        if (dist > 0.0001f)
            normal = Vector3Scale(diff, 1.0f / dist);
        else
            normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v1, v0), Vector3Subtract(v2, v0)));

        ApplyResponse(tc, rb, otherCollider, normal, penetration);
        sphereWorldPos = Vector3Add(tc.Translation, sphere.Offset); // Update for stacked contacts
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Sphere vs Sphere
// ═══════════════════════════════════════════════════════════════════════════════

void NarrowPhase::ResolveSphereSphere(::entt::registry &registry, ::entt::entity rbEntity,
                                      ::entt::entity otherEntity)
{
    auto &tc = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &s1 = registry.get<ColliderComponent>(rbEntity);
    auto &s2 = registry.get<ColliderComponent>(otherEntity);
    auto &tc2 = registry.get<TransformComponent>(otherEntity);

    Vector3 p1 = Vector3Add(tc.Translation, s1.Offset);
    Vector3 p2 = Vector3Add(tc2.Translation, s2.Offset);

    Vector3 diff = Vector3Subtract(p1, p2);
    float distSq = Vector3DotProduct(diff, diff);
    float radiusSum = s1.Radius + s2.Radius;

    if (distSq >= radiusSum * radiusSum) return;

    float dist = sqrtf(distSq);
    float penetration = radiusSum - dist;
    Vector3 normal = (dist > 0.0001f) ? Vector3Scale(diff, 1.0f / dist) : Vector3{0, 1, 0};

    ApplyResponse(tc, rb, s2, normal, penetration);
}

} // namespace CHEngine