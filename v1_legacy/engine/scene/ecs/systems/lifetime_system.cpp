#include "lifetime_system.h"
#include "engine/scene/ecs/components/utility_components.h"
#include <vector>

namespace CHEngine
{
void LifetimeSystem::Update(entt::registry &registry, float deltaTime)
{
    auto view = registry.view<LifetimeComponent>();
    std::vector<entt::entity> toDestroy;

    for (auto &&[entity, lifetime] : view.each())
    {
        lifetime.timer += deltaTime;
        if (lifetime.timer >= lifetime.lifetime && lifetime.destroyOnTimeout)
        {
            toDestroy.push_back(entity);
        }
    }

    for (auto entity : toDestroy)
    {
        registry.destroy(entity);
    }
}
} // namespace CHEngine
