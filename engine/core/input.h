#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "array"
#include "functional"
#include "mutex"
#include "raylib.h"
#include "string"
#include "unordered_map"
#include "vector"

namespace CHEngine
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

    // Action System (Abstractions)
    static bool IsActionPressed(const std::string &action);
    static bool IsActionDown(const std::string &action);
    static Vector2 GetActionAxis(const std::string &action);

    // Event Handling
    static void OnEvent(class Event &e);

    // Internal state management (called by Application)
    static void UpdateState();

private:
    static void MapDefaults();

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

    static std::vector<int> s_ActiveKeys;

    static Vector2 s_LastMousePosition;
    static Vector2 s_MouseDelta;

    static float s_MouseWheelMove;

    static bool s_IsShiftDown;
    static bool s_IsCtrlDown;
    static bool s_IsAltDown;

    static std::mutex s_InputMutex;

    // Action Mapping
    static std::unordered_map<std::string, std::vector<int>> s_ActionKeyMap;
    static std::unordered_map<std::string, std::vector<int>> s_ActionMouseMap;
};
} // namespace CHEngine

#endif // CH_INPUT_H
