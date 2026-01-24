#include "input.h"
#include "engine/core/events.h"
#include "engine/core/log.h"
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

Vector2 Input::s_LastMousePosition = {0.0f, 0.0f};
Vector2 Input::s_MouseDelta = {0.0f, 0.0f};
float Input::s_MouseWheelMove = 0.0f;

bool Input::s_IsShiftDown = false;
bool Input::s_IsCtrlDown = false;
bool Input::s_IsAltDown = false;

std::mutex Input::s_InputMutex;

// Keyboard
bool Input::IsKeyPressed(int key)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysPressedThisFrame[key];
}

bool Input::IsKeyDown(int key)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysDown[key];
}

bool Input::IsKeyReleased(int key)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return s_KeysReleasedThisFrame[key];
}

bool Input::IsKeyUp(int key)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (key < 0 || key >= MAX_KEYS)
        return false;
    return !s_KeysDown[key];
}

// Mouse Buttons
bool Input::IsMouseButtonPressed(int button)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsPressedThisFrame[button];
}

bool Input::IsMouseButtonDown(int button)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsDown[button];
}

bool Input::IsMouseButtonReleased(int button)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return s_MouseButtonsReleasedThisFrame[button];
}

bool Input::IsMouseButtonUp(int button)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (button < 0 || button >= MAX_MOUSE_BUTTONS)
        return false;
    return !s_MouseButtonsDown[button];
}

// Mouse Position
Vector2 Input::GetMousePosition()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    return s_LastMousePosition;
}

float Input::GetMouseX()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    return s_LastMousePosition.x;
}

float Input::GetMouseY()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    return s_LastMousePosition.y;
}

Vector2 Input::GetMouseDelta()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    return s_MouseDelta;
}

// Mouse Wheel
float Input::GetMouseWheelMove()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    return s_MouseWheelMove;
}

bool Input::IsActionPressed(const std::string &action)
{
    if (action == "Jump")
        return IsKeyPressed(KEY_SPACE);
    if (action == "Interact")
        return IsKeyPressed(KEY_E);
    if (action == "Teleport")
        return IsKeyPressed(KEY_T);
    return false;
}

bool Input::IsActionDown(const std::string &action)
{
    if (action == "Sprint")
        return IsKeyDown(KEY_LEFT_SHIFT);
    return false;
}

Vector2 Input::GetActionAxis(const std::string &action)
{
    if (action == "Move")
    {
        Vector2 axis = {0.0f, 0.0f};
        if (IsKeyDown(KEY_D))
            axis.x += 1.0f;
        if (IsKeyDown(KEY_A))
            axis.x -= 1.0f;
        if (IsKeyDown(KEY_W))
            axis.y += 1.0f;
        if (IsKeyDown(KEY_S))
            axis.y -= 1.0f;
        return axis;
    }
    return {0.0f, 0.0f};
}

// Internal state management
void Input::PollEvents(std::function<void(class Event &)> eventCallback)
{
    // Note: Polling still calls Raylib directly, which is correct as it runs on the MAIN thread.

    // 1. Keyboard Events
    int key = GetKeyPressed();
    while (key != 0)
    {
        OnKeyPressed(key);
        {
            std::lock_guard<std::mutex> lock(s_InputMutex);
            if (std::find(s_ActiveKeys.begin(), s_ActiveKeys.end(), key) == s_ActiveKeys.end())
            {
                s_ActiveKeys.push_back(key);
            }
        }

        KeyPressedEvent e(key, false);
        eventCallback(e);
        key = GetKeyPressed();
    }

    std::vector<int> keysToRemove;
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        auto it = s_ActiveKeys.begin();
        while (it != s_ActiveKeys.end())
        {
            int activeKey = *it;
            if (::IsKeyReleased(activeKey))
            {
                // We'll call OnKeyReleased outside the loop to handle its own locking
                keysToRemove.push_back(activeKey);
                it = s_ActiveKeys.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    for (int k : keysToRemove)
    {
        OnKeyReleased(k);
        KeyReleasedEvent e(k);
        eventCallback(e);
    }

    // 2. Mouse Events
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

    Vector2 currentMousePos = ::GetMousePosition();
    Vector2 lastPos;
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        lastPos = s_LastMousePosition;
    }

    if (currentMousePos.x != lastPos.x || currentMousePos.y != lastPos.y)
    {
        MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
        eventCallback(e);
    }

    float wheel = ::GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        eventCallback(e);
    }
}

void Input::UpdateState()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);

    s_KeysPressedThisFrame.fill(false);
    s_KeysReleasedThisFrame.fill(false);
    s_MouseButtonsPressedThisFrame.fill(false);
    s_MouseButtonsReleasedThisFrame.fill(false);

    Vector2 currentMousePos = ::GetMousePosition();
    s_MouseDelta = {currentMousePos.x - s_LastMousePosition.x,
                    currentMousePos.y - s_LastMousePosition.y};
    s_LastMousePosition = currentMousePos;
    s_MouseWheelMove = ::GetMouseWheelMove();
}

void Input::OnKeyPressed(int key)
{
    if (key >= 0 && key < MAX_KEYS)
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        s_KeysDown[key] = true;
        s_KeysPressedThisFrame[key] = true;
    }
}

void Input::OnKeyReleased(int key)
{
    if (key >= 0 && key < MAX_KEYS)
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        s_KeysDown[key] = false;
        s_KeysReleasedThisFrame[key] = true;
    }
}

void Input::OnMouseButtonPressed(int button)
{
    if (button >= 0 && button < MAX_MOUSE_BUTTONS)
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        s_MouseButtonsDown[button] = true;
        s_MouseButtonsPressedThisFrame[button] = true;
    }
}

void Input::OnMouseButtonReleased(int button)
{
    if (button >= 0 && button < MAX_MOUSE_BUTTONS)
    {
        std::lock_guard<std::mutex> lock(s_InputMutex);
        s_MouseButtonsDown[button] = false;
        s_MouseButtonsReleasedThisFrame[button] = true;
    }
}
} // namespace CHEngine
