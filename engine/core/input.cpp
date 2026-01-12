#include "input.h"

namespace CHEngine
{
// Static member initialization
std::array<bool, Input::MAX_KEYS> Input::s_KeysDown = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysPressedThisFrame = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysReleasedThisFrame = {};

std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsDown = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsPressedThisFrame = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsReleasedThisFrame = {};

Vector2 Input::s_LastMousePosition = {0.0f, 0.0f};
Vector2 Input::s_MouseDelta = {0.0f, 0.0f};

// Keyboard
bool Input::IsKeyPressed(int key)
{
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysPressedThisFrame[key];
}

bool Input::IsKeyDown(int key)
{
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysDown[key];
}

bool Input::IsKeyReleased(int key)
{
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysReleasedThisFrame[key];
}

bool Input::IsKeyUp(int key)
{
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return !s_KeysDown[key];
}

// Mouse Buttons
bool Input::IsMouseButtonPressed(int button)
{
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsPressedThisFrame[button];
}

bool Input::IsMouseButtonDown(int button)
{
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsDown[button];
}

bool Input::IsMouseButtonReleased(int button)
{
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsReleasedThisFrame[button];
}

bool Input::IsMouseButtonUp(int button)
{
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return !s_MouseButtonsDown[button];
}

// Mouse Position
Vector2 Input::GetMousePosition()
{
    return ::GetMousePosition();
}

float Input::GetMouseX()
{
    return GetMousePosition().x;
}

float Input::GetMouseY()
{
    return GetMousePosition().y;
}

Vector2 Input::GetMouseDelta()
{
    return s_MouseDelta;
}

// Mouse Wheel
float Input::GetMouseWheelMove()
{
    return ::GetMouseWheelMove();
}

// Internal state management
void Input::UpdateState()
{
    // Clear per-frame flags
    s_KeysPressedThisFrame.fill(false);
    s_KeysReleasedThisFrame.fill(false);
    s_MouseButtonsPressedThisFrame.fill(false);
    s_MouseButtonsReleasedThisFrame.fill(false);

    // Update mouse delta
    Vector2 currentMousePos = ::GetMousePosition();
    s_MouseDelta = {currentMousePos.x - s_LastMousePosition.x,
                    currentMousePos.y - s_LastMousePosition.y};
    s_LastMousePosition = currentMousePos;
}

void Input::OnKeyPressed(int key)
{
    if (key >= 0 && key < MAX_KEYS)
    {
        s_KeysDown[key] = true;
        s_KeysPressedThisFrame[key] = true;
    }
}

void Input::OnKeyReleased(int key)
{
    if (key >= 0 && key < MAX_KEYS)
    {
        s_KeysDown[key] = false;
        s_KeysReleasedThisFrame[key] = true;
    }
}

void Input::OnMouseButtonPressed(int button)
{
    if (button >= 0 && button < MAX_MOUSE_BUTTONS)
    {
        s_MouseButtonsDown[button] = true;
        s_MouseButtonsPressedThisFrame[button] = true;
    }
}

void Input::OnMouseButtonReleased(int button)
{
    if (button >= 0 && button < MAX_MOUSE_BUTTONS)
    {
        s_MouseButtonsDown[button] = false;
        s_MouseButtonsReleasedThisFrame[button] = true;
    }
}
} // namespace CHEngine
