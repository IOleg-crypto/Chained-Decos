#ifndef INPUT_H
#define INPUT_H

#include <raylib.h>

namespace CHEngine
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
        return ::GetMousePosition().x;
    }
    static float GetMouseY()
    {
        return ::GetMousePosition().y;
    }
};

} // namespace CHEngine

#endif // INPUT_H
