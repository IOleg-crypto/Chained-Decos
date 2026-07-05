#include "components/game_components.h"
#include "engine/scene/component_registry.h"
#include "thirdparty/IconsFontAwesome6.h"
#include <entt/entt.hpp>

namespace Chained
{
    void RegisterGameComponents()
    {
        ComponentRegistry::RegisterReflective<SpawnComponent>("SpawnZone", ICON_FA_LOCATION_DOT);
        ComponentRegistry::RegisterReflective<PlayerComponent>("Player", ICON_FA_USER);
    }
}
