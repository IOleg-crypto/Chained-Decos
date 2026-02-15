#ifndef CH_NARROW_PHASE_H
#define CH_NARROW_PHASE_H

#include "entt/entt.hpp"
#include <vector>

namespace CHEngine
{
class Physics;
class Scene;

class NarrowPhase
{
public:
    NarrowPhase(Physics* physics) : m_Physics(physics) {}

    void ResolveCollisions(Scene *scene, const std::vector<::entt::entity> &entities);

private:
    void ResolveBoxBox(::entt::registry &registry, ::entt::entity rbEntity,
                              ::entt::entity otherEntity);
    void ResolveBoxMesh(::entt::registry &registry, ::entt::entity rbEntity,
                               ::entt::entity otherEntity);

    void ResolveCapsuleBox(::entt::registry &registry, ::entt::entity rbEntity,
                                  ::entt::entity otherEntity);
    void ResolveCapsuleMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                   ::entt::entity otherEntity);

    void ResolveSphereBox(::entt::registry& registry, ::entt::entity rbEntity,
                                 ::entt::entity otherEntity);
    void ResolveSphereMesh(::entt::registry& registry, ::entt::entity rbEntity,
                                  ::entt::entity otherEntity);
    void ResolveSphereSphere(::entt::registry& registry, ::entt::entity rbEntity,
                                    ::entt::entity otherEntity);

private:
    Physics* m_Physics;
};
} // namespace CHEngine

#endif // CH_NARROW_PHASE_H
