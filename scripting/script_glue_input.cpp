#include "script_glue_input.h"

namespace Chained
{
void Input_GetMouseDelta(glm::vec3* outDelta)
{
    glm::vec2 delta = Core::Input::GetMouseDelta();
    *outDelta = {delta.x, delta.y, 0.0f};
}
float Input_GetMouseWheelMove()
{
    return Core::Input::GetMouseWheelMove();
}
bool Input_IsMouseButtonPressed(int button)
{
    return Core::Input::IsMouseButtonPressed(static_cast<MouseCode>(button));
}
bool Input_IsMouseButtonDown(int button)
{
    return Core::Input::IsMouseButtonDown(static_cast<MouseCode>(button));
}
bool Input_IsKeyReleased(int keyCode)
{
    return Core::Input::IsKeyReleased(static_cast<KeyCode>(keyCode));
}
bool Input_IsKeyPressed(int keyCode)
{
    return Core::Input::IsKeyPressed(static_cast<KeyCode>(keyCode));
}
bool Input_IsKeyDown(int keyCode)
{
    return Core::Input::IsKeyDown(static_cast<KeyCode>(keyCode));
}

void RegisterGlueInput()
{
    CH_ADD_INTERNAL_CALL("Input", Input_GetMouseDelta, Input_GetMouseDelta);
    CH_ADD_INTERNAL_CALL("Input", Input_GetMouseWheelMove, Input_GetMouseWheelMove);
    CH_ADD_INTERNAL_CALL("Input", Input_IsMouseButtonPressed, Input_IsMouseButtonPressed);
    CH_ADD_INTERNAL_CALL("Input", Input_IsMouseButtonDown, Input_IsMouseButtonDown);
    CH_ADD_INTERNAL_CALL("Input", Input_IsKeyReleased, Input_IsKeyReleased);
    CH_ADD_INTERNAL_CALL("Input", Input_IsKeyPressed, Input_IsKeyPressed);
    CH_ADD_INTERNAL_CALL("Input", Input_IsKeyDown, Input_IsKeyDown);
}

}