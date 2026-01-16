#include "input.h"
#include "engine/core/events.h"
#include "input_manager.h"
#include <algorithm>

namespace CHEngine
{
// Static member initialization
std::array<bool, Input::MAX_KEYS> Input::s_KeysDown = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysPressedThisFrame = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysReleasedThisFrame = {};

std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsDown = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsPressedThisFrame = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsReleasedThisFrame = {};

std::vector<int> Input::s_ActiveKeys = {};
std::unordered_map<std::string, int> Input::s_RegisteredInputActions = {};

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

void Input::RegisterAction(const std::string &name, int key)
{
    s_RegisteredInputActions[name] = key;
}

bool Input::IsActionPressed(const std::string &name)
{
    if (s_RegisteredInputActions.find(name) == s_RegisteredInputActions.end())
        return false;
    return IsKeyPressed(s_RegisteredInputActions[name]);
}

bool Input::IsActionDown(const std::string &name)
{
    if (s_RegisteredInputActions.find(name) == s_RegisteredInputActions.end())
        return false;
    return IsKeyDown(s_RegisteredInputActions[name]);
}

// Internal state management
void Input::PollEvents(std::function<void(class Event &)> eventCallback)
{
    // 1. Keyboard Events
    int key = GetKeyPressed();
    while (key != 0)
    {
        OnKeyPressed(key);
        // Only add if not already in active keys
        if (std::find(s_ActiveKeys.begin(), s_ActiveKeys.end(), key) == s_ActiveKeys.end())
        {
            s_ActiveKeys.push_back(key);
        }

        // Notify InputManager for action processing
        InputManager::ProcessKeyPressed(key);

        KeyPressedEvent e(key, false);
        eventCallback(e);
        key = GetKeyPressed();
    }

    // Check for releases without looping through the entire range
    // We only check keys that we know were previously down.
    auto it = s_ActiveKeys.begin();
    while (it != s_ActiveKeys.end())
    {
        int activeKey = *it;
        if (::IsKeyReleased(activeKey))
        {
            OnKeyReleased(activeKey);

            // Notify InputManager for action processing
            InputManager::ProcessKeyReleased(activeKey);

            KeyReleasedEvent e(activeKey);
            eventCallback(e);
            it = s_ActiveKeys.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 2. Mouse Events - Individual checks (No loop)
    auto handleMouse = [&](int button)
    {
        if (::IsMouseButtonPressed(button))
        {
            OnMouseButtonPressed(button);
            MouseButtonPressedEvent e(button);
            eventCallback(e);
        }
        if (::IsMouseButtonReleased(button))
        {
            OnMouseButtonReleased(button);
            MouseButtonReleasedEvent e(button);
            eventCallback(e);
        }
    };

    handleMouse(MOUSE_BUTTON_LEFT);
    handleMouse(MOUSE_BUTTON_RIGHT);
    handleMouse(MOUSE_BUTTON_MIDDLE);

    // Mouse Movement
    Vector2 currentMousePos = ::GetMousePosition();
    if (currentMousePos.x != s_LastMousePosition.x || currentMousePos.y != s_LastMousePosition.y)
    {
        MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
        eventCallback(e);
        // Delta and LastPos will be updated in UpdateState() called later/every frame
    }

    // Mouse Scroll
    float wheel = ::GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        eventCallback(e);
    }
}

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

    // Process axis inputs for InputManager (continuous input like WASD movement)
    InputManager::ProcessAxisInput();
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
