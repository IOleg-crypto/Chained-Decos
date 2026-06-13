#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/core/input.h"

namespace Chained
{



CH_SCRIPT_FUNC bool Input_IsKeyDown(int keyCode)
{
    return Core::Input::IsKeyDown(static_cast<KeyCode>(keyCode));
}


CH_SCRIPT_FUNC bool Input_IsKeyPressed(int keyCode)
{
    return Core::Input::IsKeyPressed(static_cast<KeyCode>(keyCode));
}


CH_SCRIPT_FUNC bool Input_IsKeyReleased(int keyCode)
{
    return Core::Input::IsKeyReleased(static_cast<KeyCode>(keyCode));
}


CH_SCRIPT_FUNC bool Input_IsMouseButtonDown(int button)
{
    return Core::Input::IsMouseButtonDown(static_cast<MouseCode>(button));
}


CH_SCRIPT_FUNC bool Input_IsMouseButtonPressed(int button)
{
    return Core::Input::IsMouseButtonPressed(static_cast<MouseCode>(button));
}


CH_SCRIPT_FUNC void Input_GetMouseDelta(glm::vec3* outDelta)
{
    glm::vec2 delta = Core::Input::GetMouseDelta();
    *outDelta = {delta.x, delta.y, 0.0f};
}


CH_SCRIPT_FUNC float Input_GetMouseWheelMove()
{
    return Core::Input::GetMouseWheelMove();
}


    void RegisterGlueInput(Coral::ManagedAssembly& assembly) {
            assembly.AddInternalCall("Chained.Input", "Input_IsKeyDown_Ptr", (void*)Input_IsKeyDown);
            assembly.AddInternalCall("Chained.Input", "Input_IsKeyPressed_Ptr", (void*)Input_IsKeyPressed);
            assembly.AddInternalCall("Chained.Input", "Input_IsKeyReleased_Ptr", (void*)Input_IsKeyReleased);
            assembly.AddInternalCall("Chained.Input", "Input_IsMouseButtonDown_Ptr", (void*)Input_IsMouseButtonDown);
            assembly.AddInternalCall("Chained.Input", "Input_IsMouseButtonPressed_Ptr", (void*)Input_IsMouseButtonPressed);
            assembly.AddInternalCall("Chained.Input", "Input_GetMouseDelta_Ptr", (void*)Input_GetMouseDelta);
            assembly.AddInternalCall("Chained.Input", "Input_GetMouseWheelMove_Ptr", (void*)Input_GetMouseWheelMove);
        }
} // namespace Chained

