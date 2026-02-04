#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/core/base.h"

namespace CHEngine
{
    // Static class for polling input state and dispatching raw raylib events to the Application.
    class Input
    {
    public:
        // Polls raylib for input changes and dispatches events to Application::OnEvent.
        static void PollEvents();

        // Direct polling API (wraps raylib)
        static bool IsKeyPressed(int key);
        static bool IsKeyDown(int key);
        static bool IsKeyReleased(int key);
        static bool IsKeyUp(int key);

        static bool IsMouseButtonPressed(int button);
        static bool IsMouseButtonDown(int button);
        static bool IsMouseButtonReleased(int button);
        static bool IsMouseButtonUp(int button);

        static Vector2 GetMousePosition();
        static Vector2 GetMouseDelta();
        static float GetMouseWheelMove();
    };
}

#endif // CH_INPUT_H
