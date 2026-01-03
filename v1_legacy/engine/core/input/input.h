#ifndef CD_ENGINE_CORE_INPUT_INPUT_H
#define CD_ENGINE_CORE_INPUT_INPUT_H

#include <raylib.h>

namespace CHEngine
{
class Input
{
public:
    static void Update();

    static bool IsKeyPressed(int keycode);
    static bool IsMouseButtonPressed(int button);
    static Vector2 GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};
} // namespace CHEngine

#endif
