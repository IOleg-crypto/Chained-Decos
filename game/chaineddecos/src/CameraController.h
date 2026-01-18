#ifndef CH_CAMERA_CONTROLLER_H
#define CH_CAMERA_CONTROLLER_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"

namespace CHEngine
{
CH_SCRIPT(CameraController){public :
                                CH_UPDATE(deltaTime){if (!HasComponent<PlayerComponent>()) return;

auto &player = GetComponent<PlayerComponent>();

// Rotation control (Right Mouse Button)
if (Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
{
    Vector2 mouseDelta = Input::GetMouseDelta();
    player.CameraYaw -= mouseDelta.x * player.LookSensitivity;
    player.CameraPitch -= mouseDelta.y * player.LookSensitivity;

    // Clamp pitch to prevent flipping
    if (player.CameraPitch > 89.0f)
        player.CameraPitch = 89.0f;
    if (player.CameraPitch < -10.0f)
        player.CameraPitch = -10.0f;
}

// Zoom control (Mouse Wheel)
float wheelMovement = Input::GetMouseWheelMove();
player.CameraDistance -= wheelMovement * 2.0f;

// Clamp distance to reasonable limits
if (player.CameraDistance < 2.0f)
    player.CameraDistance = 2.0f;
if (player.CameraDistance > 40.0f)
    player.CameraDistance = 40.0f;
} // namespace CHEngine
}
;
} // namespace CHEngine

#endif // CH_CAMERA_CONTROLLER_H
