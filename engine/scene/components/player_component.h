#ifndef CH_PLAYER_COMPONENT_H
#define CH_PLAYER_COMPONENT_H
#include "engine/scene/component_registry.h"

#include <glm/glm.hpp>

namespace Chained
{
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
}
#endif
