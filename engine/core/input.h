#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "raylib.h"
#include <string>
#include <vector>

namespace CHEngine
{
/**
 * Static wrapper for Input handling (Keyboard, Mouse)
 * Uses Raylib functions directly for state polling.
 */
class Input
{
public:
    // Keyboard
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

    // Mouse Buttons
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

    // Mouse Position
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

    // Mouse Wheel
    static float GetMouseWheelMove()
    {
        return ::GetMouseWheelMove();
    }

private:
    Input() = default;
};
} // namespace CHEngine

#endif // CH_INPUT_H
