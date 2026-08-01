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
    bool IsCheckpoint = true;
    glm::vec3 SpawnPoint = {0.0f, 0.0f, 0.0f};
    AssetHandle TextureHandle = 0;
    glm::vec3 ZoneSize = {1.0f, 1.0f, 1.0f};
    bool RenderSpawnZoneInScene = true;

    static const char* GetStaticName()
    {
        return "SpawnComponent";
    }

    struct UI
    {
        UIMeta IsActive = {.Tooltip = "Whether this spawn zone is enabled"};
        UIMeta IsCheckpoint = {.Tooltip = "Whether the spawn zone is the active checkpoint"};
        UIMeta SpawnPoint = {.Tooltip = "Offset position relative to entity where player spawns"};
        UIMeta ZoneSize = {.Tooltip = "Random spawn area dimensions (X, Y, Z) around entity position"};
        UIMeta RenderSpawnZoneInScene = {.Tooltip = "Render the zone boundaries in the viewport"};
    };
};
CH_MARK_RFL(SpawnComponent);
} // namespace Chained
#endif // CH_SPAWN_COMPONENT_H