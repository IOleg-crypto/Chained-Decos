#include "components/game_components.h"
#include "engine/scene/component_registry.h"
#include "engine/core/module_registry.h"
#include "IconsFontAwesome6.h"
#include <entt/entt.hpp>

namespace CHEngine
{
    void RegisterGameScriptBindings();

    void RegisterGameComponents()
    {
        ComponentRegistry::RegisterReflective<SpawnComponent>("SpawnZone", ICON_FA_LOCATION_DOT);
        ComponentRegistry::RegisterReflective<PlayerComponent>("Player", ICON_FA_USER);
        ComponentRegistry::RegisterReflective<RPGStatsComponent>("RPG Stats", ICON_FA_CHART_BAR);
        ComponentRegistry::RegisterReflective<SkillComponent>("Skill", ICON_FA_BOLT);
        ComponentRegistry::RegisterReflective<InventoryComponent>("Inventory", ICON_FA_BOXES_STACKED);

        // Script bindings still manually registered for now
        RegisterGameScriptBindings();
    }

    CH_REGISTER_MODULE_INIT(RegisterGameComponents);
}
