#include "components/game_components.h"
#include "engine/scene/component_registry.h"
#include "IconsFontAwesome6.h"
#include <entt/entt.hpp>

namespace Chained
{
    void RegisterGameComponents()
    {
        ComponentRegistry::RegisterReflective<SpawnComponent>("SpawnZone", ICON_FA_LOCATION_DOT);
        ComponentRegistry::RegisterReflective<PlayerComponent>("Player", ICON_FA_USER);
        ComponentRegistry::RegisterReflective<RPGStatsComponent>("RPG Stats", ICON_FA_CHART_BAR);
        ComponentRegistry::RegisterReflective<SkillComponent>("Skill", ICON_FA_BOLT);
        ComponentRegistry::RegisterReflective<InventoryComponent>("Inventory", ICON_FA_BOXES_STACKED);
    }
}
