#ifndef CH_NARROW_PHASE_H
#define CH_NARROW_PHASE_H

#include "engine/scene/components.h"
#include "entt/entt.hpp"
#include <vector>
#include <glm/glm.hpp>

namespace CHEngine
{

class NarrowPhase
{
public:
    static void ResolveCollisions(::entt::registry& registry, const std::vector<::entt::entity>& entities);

private:
    struct CapsuleSegment
    {
        glm::vec3 a, b;
        float radius;
    };

    struct WorldAABB
    {
        glm::vec3 Min, Max;
    };


    static void ApplyResponse(::entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity, TransformComponent& tc, RigidBodyComponent& rb,
                       ColliderComponent& other, glm::vec3 normal, float depth);
    static glm::vec3 ClosestPointOnSegment(glm::vec3 p, glm::vec3 a, glm::vec3 b);
    static glm::vec3 ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c);
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
