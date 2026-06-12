#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/core/input.h"

namespace Chained
{

void RegisterGlueInput() {}

CH_SCRIPT_FUNC bool Input_IsKeyDown(int keyCode)
{
    return Core::Input::IsKeyDown(keyCode);
}
CH_ADD_INTERNAL_CALL(Input, Input_IsKeyDown_Ptr, Input_IsKeyDown);

CH_SCRIPT_FUNC bool Input_IsKeyPressed(int keyCode)
{
    return Core::Input::IsKeyPressed(keyCode);
}
CH_ADD_INTERNAL_CALL(Input, Input_IsKeyPressed_Ptr, Input_IsKeyPressed);

CH_SCRIPT_FUNC bool Input_IsKeyReleased(int keyCode)
{
    return Core::Input::IsKeyReleased(keyCode);
}
CH_ADD_INTERNAL_CALL(Input, Input_IsKeyReleased_Ptr, Input_IsKeyReleased);

CH_SCRIPT_FUNC bool Input_IsMouseButtonDown(int button)
{
    return Core::Input::IsMouseButtonDown(button);
}
CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonDown_Ptr, Input_IsMouseButtonDown);

CH_SCRIPT_FUNC bool Input_IsMouseButtonPressed(int button)
{
    return Core::Input::IsMouseButtonPressed(button);
}
CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonPressed_Ptr, Input_IsMouseButtonPressed);

CH_SCRIPT_FUNC void Input_GetMouseDelta(glm::vec3* outDelta)
{
    glm::vec2 delta = Core::Input::GetMouseDelta();
    *outDelta = {delta.x, delta.y, 0.0f};
}
CH_ADD_INTERNAL_CALL(Input, Input_GetMouseDelta_Ptr, Input_GetMouseDelta);

CH_SCRIPT_FUNC float Input_GetMouseWheelMove()
{
    return Core::Input::GetMouseWheelMove();
}
CH_ADD_INTERNAL_CALL(Input, Input_GetMouseWheelMove_Ptr, Input_GetMouseWheelMove);

} // namespace Chained

