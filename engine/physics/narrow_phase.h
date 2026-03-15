#ifndef CH_NARROW_PHASE_H
#define CH_NARROW_PHASE_H

#include "engine/scene/components.h"
#include "entt/entt.hpp"
#include "raylib.h"
#include <vector>

namespace CHEngine
{
class Physics;
class Scene;

class NarrowPhase
{
public:
    static void ResolveCollisions(::entt::registry& registry, const std::vector<::entt::entity>& entities);

private:
    struct CapsuleSegment
    {
        Vector3 a, b;
        float radius;
    };

    struct WorldAABB
    {
        Vector3 min, max;
    };

    static void ApplyResponse(::entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, TransformComponent& tc, RigidBodyComponent& rb,
                       ColliderComponent& other, Vector3 normal, float depth);
    static Vector3 ClosestPointOnSegment(Vector3 p, Vector3 a, Vector3 b);
    static Vector3 ClosestPointTriangle(Vector3 p, Vector3 a, Vector3 b, Vector3 c);
    static CapsuleSegment GetCapsuleSegment(const TransformComponent& tc, const ColliderComponent& cc);
    static WorldAABB GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc);

private:
    static void ResolveBoxBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    static void ResolveBoxMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);

    static void ResolveCapsuleBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    static void ResolveCapsuleMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);

    static void ResolveSphereBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    static void ResolveSphereMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    static void ResolveSphereSphere(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
};
} // namespace CHEngine

#endif // CH_NARROW_PHASE_H
