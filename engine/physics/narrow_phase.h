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
    NarrowPhase(Physics* physics)
        : m_Physics(physics)
    {
    }

    void ResolveCollisions(Scene* scene, const std::vector<::entt::entity>& entities);

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

    static void ApplyResponse(TransformComponent& tc, RigidBodyComponent& rb, ColliderComponent& other, Vector3 normal,
                              float depth);
    static Vector3 ClosestPointOnSegment(Vector3 p, Vector3 a, Vector3 b);
    static Vector3 ClosestPointTriangle(Vector3 p, Vector3 a, Vector3 b, Vector3 c);
    static CapsuleSegment GetCapsuleSegment(const TransformComponent& tc, const ColliderComponent& cc);
    static WorldAABB GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc);

private:
    void ResolveBoxBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    void ResolveBoxMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);

    void ResolveCapsuleBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    void ResolveCapsuleMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);

    void ResolveSphereBox(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    void ResolveSphereMesh(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);
    void ResolveSphereSphere(::entt::registry& registry, ::entt::entity rbEntity, ::entt::entity otherEntity);

private:
    Physics* m_Physics;
};
} // namespace CHEngine

#endif // CH_NARROW_PHASE_H
