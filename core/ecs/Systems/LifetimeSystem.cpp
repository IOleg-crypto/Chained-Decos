#include "LifetimeSystem.h"
#include "core/ecs/Components/UtilityComponents.h"
#include "core/ecs/ECSRegistry.h"
#include <vector>

namespace LifetimeSystem
{

void Update(float deltaTime)
{
    auto view = REGISTRY.view<LifetimeComponent>();

    std::vector<entt::entity> toDestroy;

    for (auto [entity, lifetime] : view.each())
    {
        lifetime.timer += deltaTime;

        if (lifetime.timer >= lifetime.lifetime && lifetime.destroyOnTimeout)
        {
            toDestroy.push_back(entity);
        }
    }

    // Destroy entities after iteration
    for (auto entity : toDestroy)
    {
        REGISTRY.destroy(entity);
    }
}

} // namespace LifetimeSystem
