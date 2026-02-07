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
        
        ColliderComponent rigidBodyCollider;
        if (registry.all_of<ColliderComponent>(rbEntity))
            rigidBodyCollider = registry.get<ColliderComponent>(rbEntity);
        else
            rigidBodyCollider.Type = ColliderType::Box; 
            
        if (!registry.all_of<ColliderComponent>(rbEntity))
            continue; 

        rigidBodyCollider = registry.get<ColliderComponent>(rbEntity);

        for (auto otherEntity : colliders)
        {
            if (rbEntity == otherEntity)
                continue;

            auto &otherCollider = colliders.get<ColliderComponent>(otherEntity);
            if (!otherCollider.Enabled)
                continue;

            if (otherCollider.Type == ColliderType::Box)
            {
                if (rigidBodyCollider.Type == ColliderType::Box)
                    ResolveBoxBox(registry, rbEntity, otherEntity);
                else if (rigidBodyCollider.Type == ColliderType::Capsule)
                    ResolveCapsuleBox(registry, rbEntity, otherEntity);
            }
            else if (otherCollider.Type == ColliderType::Mesh && otherCollider.BVHRoot)
            {
                if (rigidBodyCollider.Type == ColliderType::Box)
                    ResolveBoxMesh(registry, rbEntity, otherEntity);
                else if (rigidBodyCollider.Type == ColliderType::Capsule)
                    ResolveCapsuleMesh(registry, rbEntity, otherEntity);
            }
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

void NarrowPhase::ResolveBoxMesh(entt::registry &registry, entt::entity rbEntity,
                                 entt::entity otherEntity)
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

void NarrowPhase::ResolveCapsuleBox(::entt::registry &registry, ::entt::entity rbEntity,
                                    ::entt::entity otherEntity)
{
    auto &transform = registry.get<TransformComponent>(rbEntity);
    auto &rb = registry.get<RigidBodyComponent>(rbEntity);
    auto &capsule = registry.get<ColliderComponent>(rbEntity);

    auto &otherTransform = registry.get<TransformComponent>(otherEntity);
    auto &box = registry.get<ColliderComponent>(otherEntity);
    
    Vector3 capsulePos = Vector3Add(transform.Translation, capsule.Offset);
    float capsuleHeight = capsule.Height;
    float capsuleRadius = capsule.Radius;
    
    // Segment logic (Y-up)
    float halfSegment = fmaxf(0.0f, (capsuleHeight * 0.5f) - capsuleRadius);
    Vector3 segA = { capsulePos.x, capsulePos.y - halfSegment, capsulePos.z };
    Vector3 segB = { capsulePos.x, capsulePos.y + halfSegment, capsulePos.z };

    // Box details
    Vector3 otherScale = otherTransform.Scale;
    Vector3 boxMin = Vector3Add(otherTransform.Translation, Vector3Multiply(box.Offset, otherScale));
    Vector3 boxMax = Vector3Add(boxMin, Vector3Multiply(box.Size, otherScale));

    Vector3 points[] = { segA, segB, capsulePos };
    
    float minPenetration = FLT_MAX;
    Vector3 bestNormal = {0,0,0};
    bool collided = false;

    // Helper: Closest point on AABB to point P
    auto ClosestPointAABB = [&](Vector3 p) {
        return Vector3{
            fmaxf(boxMin.x, fminf(p.x, boxMax.x)),
            fmaxf(boxMin.y, fminf(p.y, boxMax.y)),
            fmaxf(boxMin.z, fminf(p.z, boxMax.z))
        };
    };

    // Test segment endpoints and center
    Vector3 boxCenter = Vector3Scale(Vector3Add(boxMin, boxMax), 0.5f);
    
    // Project boxCenter onto line segment AB to find dynamic closest point
    Vector3 ab = Vector3Subtract(segB, segA);
    float t = Vector3DotProduct(Vector3Subtract(boxCenter, segA), ab) / Vector3DotProduct(ab, ab);
    if (isnan(t)) t = 0.5f; // Handle zero length segment
    t = fmaxf(0.0f, fminf(1.0f, t));
    Vector3 closestOnSeg = Vector3Add(segA, Vector3Scale(ab, t));
    
    Vector3 testPoints[] = { segA, segB, closestOnSeg };
    
    for (int i=0; i<3; i++)
    {
        Vector3 p = testPoints[i];
        Vector3 q = ClosestPointAABB(p);
        
        Vector3 diff = Vector3Subtract(p, q);
        float distSq = Vector3DotProduct(diff, diff);
        
        if (distSq < capsuleRadius * capsuleRadius)
        {
            float dist = sqrtf(distSq);
            float penetration = capsuleRadius - dist;
            
            // Normalize normal
            Vector3 normal = {0,1,0};
            if (dist > 0.0001f)
                normal = Vector3Scale(diff, 1.0f/dist);
            
            if (!collided || penetration > minPenetration)
            {
                minPenetration = penetration;
                bestNormal = normal;
                collided = true;
            }
        }
    }
    
    if (collided && minPenetration > 0.0f)
    {
        // Push RB
        transform.Translation = Vector3Add(transform.Translation, Vector3Scale(bestNormal, minPenetration));
        
        // Grounding
        if (bestNormal.y > 0.5f)
        {
            rb.IsGrounded = true;
            if (rb.Velocity.y < 0) rb.Velocity.y = 0;
        }
        else if (bestNormal.y < -0.5f)
        {
            if (rb.Velocity.y > 0) rb.Velocity.y = 0;
        }
        
        // Friction / Slide
         float dot = Vector3DotProduct(rb.Velocity, bestNormal);
         if (dot < 0.0f)
            rb.Velocity = Vector3Subtract(rb.Velocity, Vector3Scale(bestNormal, dot));
            
        box.IsColliding = true;
    }
}

void NarrowPhase::ResolveCapsuleMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                     ::entt::entity otherEntity)
{
    auto &entityTransform = registry.get<TransformComponent>(rbEntity);
    auto &rigidBody = registry.get<RigidBodyComponent>(rbEntity);
    auto &capsule = registry.get<ColliderComponent>(rbEntity);

    auto &otherTransform = registry.get<TransformComponent>(otherEntity);
    auto &otherCollider = registry.get<ColliderComponent>(otherEntity);

    Matrix meshMatrix = otherTransform.GetTransform();
    Matrix invMeshMatrix = MatrixInvert(meshMatrix);

    auto ClosestPointOnSegment = [&](Vector3 p, Vector3 a, Vector3 b) {
        Vector3 ab = Vector3Subtract(b, a);
        float t = Vector3DotProduct(Vector3Subtract(p, a), ab) / Vector3DotProduct(ab, ab);
        if (isnan(t)) t = 0.5f;
        t = fmaxf(0.0f, fminf(1.0f, t));
        return Vector3Add(a, Vector3Scale(ab, t));
    };

    auto ClosestPointTriangle = [&](Vector3 p, Vector3 a, Vector3 b, Vector3 c) {
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

        float vc = d1*d4 - d3*d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return Vector3Add(a, Vector3Scale(ab, v));
        }

        Vector3 cp = Vector3Subtract(p, c);
        float d5 = Vector3DotProduct(ab, cp);
        float d6 = Vector3DotProduct(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return c;

        float vb = d5*d2 - d1*d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return Vector3Add(a, Vector3Scale(ac, w));
        }

        float va = d3*d6 - d5*d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return Vector3Add(b, Vector3Scale(Vector3Subtract(c, b), w));
        }

        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return Vector3Add(a, Vector3Add(Vector3Scale(ab, v), Vector3Scale(ac, w)));
    };

    Vector3 capsulePos = Vector3Add(entityTransform.Translation, capsule.Offset);
    float capsuleHeight = capsule.Height;
    float capsuleRadius = capsule.Radius;
    float halfSegment = fmaxf(0.0f, (capsuleHeight * 0.5f) - capsuleRadius);
    Vector3 segA = { capsulePos.x, capsulePos.y - halfSegment, capsulePos.z };
    Vector3 segB = { capsulePos.x, capsulePos.y + halfSegment, capsulePos.z };

    // AABB for query (World Space -> Local Space)
    Vector3 localSegA = Vector3Transform(segA, invMeshMatrix);
    Vector3 localSegB = Vector3Transform(segB, invMeshMatrix);
    
    Vector3 scale = otherTransform.Scale;
    float maxScale = fmaxf(scale.x, fmaxf(scale.y, scale.z));
    float localRadius = capsuleRadius / maxScale; 
    
    Vector3 minSeg = Vector3Min(localSegA, localSegB);
    Vector3 maxSeg = Vector3Max(localSegA, localSegB);
    BoundingBox queryBox;
    queryBox.min = { minSeg.x - localRadius, minSeg.y - localRadius, minSeg.z - localRadius };
    queryBox.max = { maxSeg.x + localRadius, maxSeg.y + localRadius, maxSeg.z + localRadius };

    std::vector<const CollisionTriangle*> candidates;
    otherCollider.BVHRoot->QueryAABB(queryBox, candidates);

    if (candidates.empty()) return;

    for (const auto* tri : candidates)
    {
        Vector3 v0 = Vector3Transform(tri->v0, meshMatrix);
        Vector3 v1 = Vector3Transform(tri->v1, meshMatrix);
        Vector3 v2 = Vector3Transform(tri->v2, meshMatrix);

        Vector3 testPoints[] = { segA, segB, capsulePos }; 
        
        for(Vector3 p : testPoints)
        {
            Vector3 closestOnTri = ClosestPointTriangle(p, v0, v1, v2);
            Vector3 diff = Vector3Subtract(p, closestOnTri);
            
            Vector3 closestOnSeg = ClosestPointOnSegment(closestOnTri, segA, segB);
            Vector3 realDiff = Vector3Subtract(closestOnSeg, closestOnTri);
            float realDistSq = Vector3DotProduct(realDiff, realDiff);
            
            if (realDistSq < capsuleRadius * capsuleRadius)
            {
               float realDist = sqrtf(realDistSq);
               float penetration = capsuleRadius - realDist;
               
               Vector3 normal = {0,1,0};
               if (realDist > 0.0001f)
                   normal = Vector3Scale(realDiff, 1.0f/realDist);
               else    
                   normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v1, v0), Vector3Subtract(v2, v0))); 

               entityTransform.Translation = Vector3Add(entityTransform.Translation, Vector3Scale(normal, penetration * 1.0f)); 
               
               // Update capsule pos
               capsulePos = Vector3Add(entityTransform.Translation, capsule.Offset);
               segA = { capsulePos.x, capsulePos.y - halfSegment, capsulePos.z };
               segB = { capsulePos.x, capsulePos.y + halfSegment, capsulePos.z };
               
               if (normal.y > 0.45f)
               {
                   rigidBody.IsGrounded = true;
                   if (rigidBody.Velocity.y < 0) rigidBody.Velocity.y = 0;
               }
               
               float dot = Vector3DotProduct(rigidBody.Velocity, normal);
               if (dot < 0.0f)
                  rigidBody.Velocity = Vector3Subtract(rigidBody.Velocity, Vector3Scale(normal, dot));
                  
               otherCollider.IsColliding = true;
            }
        }
    }
}

} // namespace CHEngine