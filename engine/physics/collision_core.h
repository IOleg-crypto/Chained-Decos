#ifndef CH_COLLISION_CORE_H
#define CH_COLLISION_CORE_H

#include "engine/scene/components.h"
#include "entt/entt.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace Chained
{
    struct Ray
    {
        glm::vec3 position;
        glm::vec3 direction;
    };
// Narrow-phase collision response helpers for the physics step.
class CollisionCore
{
public:
    struct Contact
    {
        entt::entity BodyA;
        entt::entity BodyB;
        glm::vec3 Normal;
        float Depth;
    };

    static void ResolveCollisions(entt::registry& registry, const std::vector<entt::entity>& entities);

private:
    static void GenerateContacts(entt::registry& registry, const std::vector<entt::entity>& entities,
                                 std::vector<Contact>& contacts);
    static void SolveContacts(entt::registry& registry, std::vector<Contact>& contacts);

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

    static void ApplyResponse(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                              TransformComponent& tc, RigidBodyComponent& rb, ColliderComponent& other,
                              glm::vec3 normal, float depth);

    static glm::vec3 ClosestPointOnSegment(glm::vec3 p, glm::vec3 a, glm::vec3 b);
    static glm::vec3 ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c);
    static CapsuleSegment GetCapsuleSegment(const TransformComponent& tc, const ColliderComponent& cc);
    static WorldAABB GetWorldAABB(const TransformComponent& tc, const ColliderComponent& cc);

    static void ResolveBoxBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                              std::vector<Contact>& contacts);
    static void ResolveBoxMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                               std::vector<Contact>& contacts);
    static void ResolveCapsuleBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                  std::vector<Contact>& contacts);
    static void ResolveCapsuleMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                   std::vector<Contact>& contacts);
    static void ResolveSphereBox(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                 std::vector<Contact>& contacts);
    static void ResolveSphereMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                  std::vector<Contact>& contacts);
    static void ResolveSphereSphere(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                    std::vector<Contact>& contacts);
    static void ResolveMeshMesh(entt::registry& registry, entt::entity rbEntity, entt::entity otherEntity,
                                std::vector<Contact>& contacts);
};
} // namespace Chained

#endif // CH_COLLISION_CORE_H
