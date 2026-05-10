#include "components/game_components.h"
#include "engine/scene/component_registry.h"
#include "engine/core/module_registry.h"
#include "IconsFontAwesome6.h"
#include <entt/entt.hpp>

namespace CHEngine
{
    void RegisterGameScriptBindings();

    CH_REGISTER_COMPONENT(SpawnComponent, "SpawnZone", ICON_FA_LOCATION_DOT);
    CH_REGISTER_COMPONENT(PlayerComponent, "Player", ICON_FA_USER);
    CH_REGISTER_COMPONENT(SceneTransitionComponent, "SceneTransition", ICON_FA_DOOR_OPEN);
    CH_REGISTER_COMPONENT(RPGStatsComponent, "RPG Stats", ICON_FA_CHART_BAR);
    CH_REGISTER_COMPONENT(SkillComponent, "Skill", ICON_FA_BOLT);
    CH_REGISTER_COMPONENT(InventoryComponent, "Inventory", ICON_FA_BOXES_STACKED);

    void RegisterGameComponents()
    {
        // Script bindings still manually registered for now
        RegisterGameScriptBindings();
    }

    CH_REGISTER_MODULE_INIT(RegisterGameComponents);
}
