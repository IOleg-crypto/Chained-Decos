#ifndef CH_NARROW_PHASE_H
#define CH_NARROW_PHASE_H

#include "entt/entt.hpp"
#include <vector>

namespace CHEngine
{
class Scene;

class NarrowPhase
{
public:
    static void ResolveCollisions(Scene *scene, const std::vector<::entt::entity> &entities);

private:
    static void ResolveBoxBox(::entt::registry &registry, ::entt::entity rbEntity,
                              ::entt::entity otherEntity);
    static void ResolveBoxMesh(::entt::registry &registry, ::entt::entity rbEntity,
                               ::entt::entity otherEntity);

    static void ResolveCapsuleBox(::entt::registry &registry, ::entt::entity rbEntity,
                                  ::entt::entity otherEntity);
    static void ResolveCapsuleMesh(::entt::registry &registry, ::entt::entity rbEntity,
                                   ::entt::entity otherEntity);
};
} // namespace CHEngine

#endif // CH_NARROW_PHASE_H
