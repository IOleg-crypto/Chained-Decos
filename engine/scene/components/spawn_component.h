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
    glm::vec3 SpawnPoint = {0.0f, 0.0f, 0.0f};

    static const char* GetStaticName()
    {
        return "SpawnComponent";
    }

    struct UI
    {
        UIMeta IsActive = {.Tooltip = "Whether the spawn zone is active"};
        UIMeta ZoneSize = {.Tooltip = "Size of the generation zone (X, Y, Z)"};
        UIMeta RenderSpawnZoneInScene = {.Tooltip = "Render the zone boundaries in the viewport"};
        UIMeta SpawnPoint = {.Tooltip = "Local or world spawn point position"};
    };
};
CH_MARK_RFL(SpawnComponent);
} // namespace Chained
#endif // CH_SPAWN_COMPONENT_H