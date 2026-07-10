#ifndef CH_GAME_COMPONENTS_H
#define CH_GAME_COMPONENTS_H

#include "engine/assets/asset.h"
#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <string>

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

// --- PLAYER COMPONENT ---
struct PlayerComponent
{
    float MovementSpeed = 15.0f;
    float JumpForce = 10.0f;
    float LookSensitivity = 0.9f;

    static const char* GetStaticName()
    {
        return "PlayerComponent";
    }

    struct UI
    {
        UIMeta MovementSpeed = {.Tooltip = "Movement speed of the player"};
        UIMeta JumpForce = {.Tooltip = "Jump force upward impulse"};
        UIMeta LookSensitivity = {.Tooltip = "Mouse or camera look sensitivity"};
    };
};
CH_MARK_RFL(PlayerComponent);




} // namespace Chained

#endif // CH_GAME_COMPONENTS_H