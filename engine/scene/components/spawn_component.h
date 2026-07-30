#ifndef CH_SPAWN_COMPONENT_H
#define CH_SPAWN_COMPONENT_H
#include "engine/assets/asset.h"
#include "engine/scene/component_registry.h"
#include <glm/glm.hpp>

namespace Chained
{
// --- SPAWN COMPONENT ---
struct SpawnComponent
{
    bool IsActive = true;
    AssetHandle TextureHandle = 0;
    glm::vec3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;

    static const char* GetStaticName()
    {
        return "SpawnComponent";
    }

    struct UI
    {
        UIMeta IsActive = {.Tooltip = "Whether the spawn zone is active"};
        UIMeta ZoneSize = {.Tooltip = "Random spawn area dimensions (X, Y, Z) around entity position"};
        UIMeta RenderSpawnZoneInScene = {.Tooltip = "Render the zone boundaries in the viewport"};
    };
};
CH_MARK_RFL(SpawnComponent);
} // namespace Chained
#endif // CH_SPAWN_COMPONENT_H