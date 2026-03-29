#include "script_glue_internal.h"

namespace CHEngine
{

CH_SCRIPT_FUNC bool Input_IsKeyDown(int keyCode)
{
    return Input::IsKeyDown(keyCode);
}
CH_SCRIPT_FUNC bool Input_IsKeyPressed(int keyCode)
{
    return Input::IsKeyPressed(keyCode);
}
CH_SCRIPT_FUNC bool Input_IsKeyReleased(int keyCode)
{
    return Input::IsKeyReleased(keyCode);
}
CH_SCRIPT_FUNC bool Input_IsMouseButtonDown(int button)
{
    return Input::IsMouseButtonDown(button);
}
CH_SCRIPT_FUNC bool Input_IsMouseButtonPressed(int button)
{
    return Input::IsMouseButtonPressed(button);
}

CH_SCRIPT_FUNC void Input_GetMouseDelta(glm::vec3* outDelta)
{
    glm::vec2 delta = Input::GetMouseDelta();
    *outDelta = {delta.x, delta.y, 0.0f};
}
CH_SCRIPT_FUNC float Input_GetMouseWheelMove()
{
    return Input::GetMouseWheelMove();
}

void RegisterInputInternalCalls(Coral::ManagedAssembly& assembly)
{
#define CH_ADD_INTERNAL_CALL(className, fieldName, funcPtr)                                                            \
    assembly.AddInternalCall("CHEngine." #className, #fieldName, (void*)funcPtr)

    CH_ADD_INTERNAL_CALL(Input, Input_IsKeyDown_Ptr, Input_IsKeyDown);
    CH_ADD_INTERNAL_CALL(Input, Input_IsKeyPressed_Ptr, Input_IsKeyPressed);
    CH_ADD_INTERNAL_CALL(Input, Input_IsKeyReleased_Ptr, Input_IsKeyReleased);
    CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonDown_Ptr, Input_IsMouseButtonDown);
    CH_ADD_INTERNAL_CALL(Input, Input_IsMouseButtonPressed_Ptr, Input_IsMouseButtonPressed);
    CH_ADD_INTERNAL_CALL(Input, Input_GetMouseDelta_Ptr, Input_GetMouseDelta);
    CH_ADD_INTERNAL_CALL(Input, Input_GetMouseWheelMove_Ptr, Input_GetMouseWheelMove);

#undef CH_ADD_INTERNAL_CALL
}

} // namespace CHEngine
