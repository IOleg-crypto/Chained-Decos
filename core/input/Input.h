#ifndef INPUT_H
#define INPUT_H

#include <functional>
#include <map>
#include <raylib.h>

namespace CHEngine
{
class Input
{
public:
    static void Update();

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

    static Vector2 GetMouseDelta()
    {
        return ::GetMouseDelta();
    }
    static float GetMouseWheelMove()
    {
        return ::GetMouseWheelMove();
    }

    static void RegisterAction(int key, const std::function<void()> &action);

private:
    static std::map<int, std::function<void()>> s_Actions;
};
} // namespace CHEngine

#endif // INPUT_H
