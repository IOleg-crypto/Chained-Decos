#include "script_glue_input.h"
#include "engine/core/log.h"

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
uint32_t Input_IsMouseButtonPressed(int button)
{
    return Core::Input::IsMouseButtonPressed(static_cast<MouseCode>(button)) ? 1 : 0;
}
uint32_t Input_IsMouseButtonDown(int button)
{
    return Core::Input::IsMouseButtonDown(static_cast<MouseCode>(button)) ? 1 : 0;
}
uint32_t Input_IsKeyReleased(int keyCode)
{
    return Core::Input::IsKeyReleased(static_cast<KeyCode>(keyCode)) ? 1 : 0;
}
uint32_t Input_IsKeyPressed(int keyCode)
{
    return Core::Input::IsKeyPressed(static_cast<KeyCode>(keyCode)) ? 1 : 0;
}
uint32_t Input_IsKeyDown(int keyCode)
{
    bool result = Core::Input::IsKeyDown(static_cast<KeyCode>(keyCode));
    if (keyCode == 23) // W key
        CH_CORE_TRACE("[Diag Input_IsKeyDown] keyCode={} result={}", keyCode, result);
    return result ? 1 : 0;
}

} // namespace Chained