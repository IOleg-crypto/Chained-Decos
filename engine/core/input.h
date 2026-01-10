#ifndef CH_INPUT_H
#define CH_INPUT_H

#include <array>
#include <raylib.h>


namespace CH
{
class Input
{
public:
    // Keyboard
    static bool IsKeyPressed(int key);
    static bool IsKeyDown(int key);
    static bool IsKeyReleased(int key);
    static bool IsKeyUp(int key);

    // Mouse Buttons
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonDown(int button);
    static bool IsMouseButtonReleased(int button);
    static bool IsMouseButtonUp(int button);

    // Mouse Position
    static Vector2 GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
    static Vector2 GetMouseDelta();

    // Mouse Wheel
    static float GetMouseWheelMove();

    // Internal state management (called by Application)
    static void UpdateState();
    static void OnKeyPressed(int key);
    static void OnKeyReleased(int key);
    static void OnMouseButtonPressed(int button);
    static void OnMouseButtonReleased(int button);

private:
    Input() = default;

    static constexpr int MAX_KEYS = 512;
    static constexpr int MAX_MOUSE_BUTTONS = 7;

    static std::array<bool, MAX_KEYS> s_KeysDown;
    static std::array<bool, MAX_KEYS> s_KeysPressedThisFrame;
    static std::array<bool, MAX_KEYS> s_KeysReleasedThisFrame;

    static std::array<bool, MAX_MOUSE_BUTTONS> s_MouseButtonsDown;
    static std::array<bool, MAX_MOUSE_BUTTONS> s_MouseButtonsPressedThisFrame;
    static std::array<bool, MAX_MOUSE_BUTTONS> s_MouseButtonsReleasedThisFrame;

    static Vector2 s_LastMousePosition;
    static Vector2 s_MouseDelta;
};
} // namespace CH

#endif // CH_INPUT_H
