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
    Dynamics() = default;

    void Update(Scene* scene, const std::vector<entt::entity>& entities, float deltaTime);

private:
    void ApplyGravity(entt::registry& registry, entt::entity entity, float gravity, float deltaTime);
    void IntegrateVelocity(entt::registry& registry, entt::entity entity, float deltaTime);
    void LogDiagnostics(entt::registry& registry, entt::entity entity);
};
} // namespace CHEngine

#endif // CH_DYNAMICS_H
