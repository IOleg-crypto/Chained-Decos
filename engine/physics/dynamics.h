#ifndef CH_DYNAMICS_H
#define CH_DYNAMICS_H

#include "entt/entt.hpp"
#include <vector>

namespace CHEngine
{
class Scene;

class Dynamics
{
public:
    static void Update(::entt::registry& registry, const std::vector<entt::entity>& entities, float deltaTime);

private:
    static void ApplyGravity(entt::registry& registry, entt::entity entity, float gravity, float deltaTime);
    static void IntegrateVelocity(entt::registry& registry, entt::entity entity, float deltaTime);
};
} // namespace CHEngine

#endif // CH_DYNAMICS_H
