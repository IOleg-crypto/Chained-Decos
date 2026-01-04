#ifndef CH_INPUT_H
#define CH_INPUT_H

#include <raylib.h>

namespace CH
{
class Input
{
public:
    static bool IsKeyPressed(int key)
    {
        return ::IsKeyPressed(key);
    }
    static bool IsKeyDown(int key)
    {
        return ::IsKeyDown(key);
    }
    static bool IsKeyReleased(int key)
    {
        return ::IsKeyReleased(key);
    }
    static bool IsKeyUp(int key)
    {
        return ::IsKeyUp(key);
    }

    static bool IsMouseButtonPressed(int button)
    {
        return ::IsMouseButtonPressed(button);
    }
    static bool IsMouseButtonDown(int button)
    {
        return ::IsMouseButtonDown(button);
    }
    static bool IsMouseButtonReleased(int button)
    {
        return ::IsMouseButtonReleased(button);
    }
    static bool IsMouseButtonUp(int button)
    {
        return ::IsMouseButtonUp(button);
    }

    static Vector2 GetMousePosition()
    {
        return ::GetMousePosition();
    }
    static float GetMouseX()
    {
        return GetMousePosition().x;
    }
    static float GetMouseY()
    {
        return GetMousePosition().y;
    }

    static float GetMouseWheelMove()
    {
        return ::GetMouseWheelMove();
    }

private:
    Input() = default;
};
} // namespace CH

#endif // CH_INPUT_H
