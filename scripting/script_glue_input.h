#ifndef SCRIPT_GLUE_INPUT_H
#define SCRIPT_GLUE_INPUT_H
#include "engine/core/input.h"
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"


namespace Chained
{

CH_SCRIPT_FUNC bool Input_IsKeyDown(int keyCode);

CH_SCRIPT_FUNC bool Input_IsKeyPressed(int keyCode);

CH_SCRIPT_FUNC bool Input_IsKeyReleased(int keyCode);

CH_SCRIPT_FUNC bool Input_IsMouseButtonDown(int button);

CH_SCRIPT_FUNC bool Input_IsMouseButtonPressed(int button);

CH_SCRIPT_FUNC void Input_GetMouseDelta(glm::vec3* outDelta);

CH_SCRIPT_FUNC float Input_GetMouseWheelMove();

void RegisterGlueInput();

} // namespace Chained
#endif