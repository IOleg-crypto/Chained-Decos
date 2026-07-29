#include "script_glue_input.h"
#include "engine/core/log.h"

namespace Chained
{
void Input_GetMouseDelta(float* outX, float* outY)
{
    glm::vec2 delta = Core::Input::GetMouseDelta();
    if (outX)
    {
        *outX = delta.x;
    }
    if (outY)
    {
        *outY = delta.y;
    }
}
float Input_GetMouseWheelMove()
{
    return Core::Input::GetMouseWheelMove();
}
int Input_IsMouseButtonPressed(int button)
{
    return Core::Input::IsMouseButtonPressed(static_cast<MouseCode>(button)) ? 1 : 0;
}
int Input_IsMouseButtonDown(int button)
{
    return Core::Input::IsMouseButtonDown(static_cast<MouseCode>(button)) ? 1 : 0;
}
int Input_IsKeyReleased(int keyCode)
{
    return Core::Input::IsKeyReleased(static_cast<KeyCode>(keyCode)) ? 1 : 0;
}
int Input_IsKeyPressed(int keyCode)
{
    return Core::Input::IsKeyPressed(static_cast<KeyCode>(keyCode)) ? 1 : 0;
}
int Input_IsKeyDown(int keyCode)
{
    return Core::Input::IsKeyDown(static_cast<KeyCode>(keyCode)) ? 1 : 0;
}

} // namespace Chained