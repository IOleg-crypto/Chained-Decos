#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/core/base.h"
#include "key_codes.h"
#include "mouse_codes.h"

namespace CHEngine
{
// Static class for polling input state and dispatching raw raylib events to the Application.
class Input
{
public:
    // Polls raylib for input changes and dispatches events to Application::OnEvent.
    static void PollEvents();

    // Direct polling API (wraps raylib)
    static bool IsKeyPressed(KeyCode key);
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyReleased(KeyCode key);
    static bool IsKeyUp(KeyCode key);

    static bool IsMouseButtonPressed(MouseCode button);
    static bool IsMouseButtonDown(MouseCode button);
    static bool IsMouseButtonReleased(MouseCode button);
    static bool IsMouseButtonUp(MouseCode button);

    static Vector2 GetMousePosition();
    static Vector2 GetMouseDelta();
    static float GetMouseWheelMove();
};
} // namespace CHEngine

#endif // CH_INPUT_H
