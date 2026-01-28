#include "input.h"
#include "algorithm"
#include "engine/core/events.h"
#include "engine/core/log.h"


namespace CHEngine
{
// Static member initialization
std::array<bool, Input::MAX_KEYS> Input::s_KeysDown = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysPressedThisFrame = {};
std::array<bool, Input::MAX_KEYS> Input::s_KeysReleasedThisFrame = {};

std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsDown = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsPressedThisFrame = {};
std::array<bool, Input::MAX_MOUSE_BUTTONS> Input::s_MouseButtonsReleasedThisFrame = {};

std::vector<int> Input::s_ActiveKeys =
    {}; // This member is no longer used with the new event system, but keeping it for now as it's
        // not explicitly removed.

Vector2 Input::s_LastMousePosition = {0.0f, 0.0f};
Vector2 Input::s_MouseDelta = {0.0f, 0.0f};
float Input::s_MouseWheelMove = 0.0f;

bool Input::s_IsShiftDown = false; // This member is no longer used with the new event system, but
                                   // keeping it for now as it's not explicitly removed.
bool Input::s_IsCtrlDown = false;  // This member is no longer used with the new event system, but
                                   // keeping it for now as it's not explicitly removed.
bool Input::s_IsAltDown = false;

std::mutex Input::s_InputMutex;

std::unordered_map<std::string, std::vector<int>> Input::s_ActionKeyMap = {};
std::unordered_map<std::string, std::vector<int>> Input::s_ActionMouseMap = {};
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

// Action System
bool Input::IsActionPressed(const std::string &action)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (s_ActionKeyMap.count(action))
    {
        for (int key : s_ActionKeyMap[action])
            if (s_KeysPressedThisFrame[key])
                return true;
    }
    if (s_ActionMouseMap.count(action))
    {
        for (int btn : s_ActionMouseMap[action])
            if (s_MouseButtonsPressedThisFrame[btn])
                return true;
    }
    return false;
}

bool Input::IsActionDown(const std::string &action)
{
    std::lock_guard<std::mutex> lock(s_InputMutex);
    if (s_ActionKeyMap.count(action))
    {
        for (int key : s_ActionKeyMap[action])
            if (s_KeysDown[key])
                return true;
    }
    if (s_ActionMouseMap.count(action))
    {
        for (int btn : s_ActionMouseMap[action])
            if (s_MouseButtonsDown[btn])
                return true;
    }
    return false;
}

Vector2 Input::GetActionAxis(const std::string &action)
{
    // Basic mapping: "MoveForward" -> W (Y=1), S (Y=-1)
    // For now returning {0,0} as we need a more robust axis mapping system
    return {0, 0};
}

void Input::MapDefaults()
{
    s_ActionKeyMap["Jump"] = {KEY_SPACE};
    s_ActionKeyMap["Horizontal"] = {KEY_A, KEY_D};
    s_ActionKeyMap["Vertical"] = {KEY_W, KEY_S};
}

// Internal state management
void Input::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(
        [](KeyPressedEvent &event)
        {
            int key = event.GetKeyCode();
            if (key >= 0 && key < MAX_KEYS)
            {
                std::lock_guard<std::mutex> lock(s_InputMutex);
                s_KeysDown[key] = true;
                s_KeysPressedThisFrame[key] = true;
            }
            return false;
        });

    dispatcher.Dispatch<KeyReleasedEvent>(
        [](KeyReleasedEvent &event)
        {
            int key = event.GetKeyCode();
            if (key >= 0 && key < MAX_KEYS)
            {
                std::lock_guard<std::mutex> lock(s_InputMutex);
                s_KeysDown[key] = false;
                s_KeysReleasedThisFrame[key] = true;
            }
            return false;
        });

    dispatcher.Dispatch<MouseButtonPressedEvent>(
        [](MouseButtonPressedEvent &event)
        {
            int button = event.GetMouseButton();
            if (button >= 0 && button < MAX_MOUSE_BUTTONS)
            {
                std::lock_guard<std::mutex> lock(s_InputMutex);
                s_MouseButtonsDown[button] = true;
                s_MouseButtonsPressedThisFrame[button] = true;
            }
            return false;
        });

    dispatcher.Dispatch<MouseButtonReleasedEvent>(
        [](MouseButtonReleasedEvent &event)
        {
            int button = event.GetMouseButton();
            if (button >= 0 && button < MAX_MOUSE_BUTTONS)
            {
                std::lock_guard<std::mutex> lock(s_InputMutex);
                s_MouseButtonsDown[button] = false;
                s_MouseButtonsReleasedThisFrame[button] = true;
            }
            return false;
        });

    dispatcher.Dispatch<MouseMovedEvent>(
        [](MouseMovedEvent &event)
        {
            std::lock_guard<std::mutex> lock(s_InputMutex);
            Vector2 currentPos = {event.GetX(), event.GetY()};
            s_MouseDelta = {currentPos.x - s_LastMousePosition.x,
                            currentPos.y - s_LastMousePosition.y};
            s_LastMousePosition = currentPos;
            return false;
        });

    dispatcher.Dispatch<MouseScrolledEvent>(
        [](MouseScrolledEvent &event)
        {
            std::lock_guard<std::mutex> lock(s_InputMutex);
            s_MouseWheelMove = event.GetYOffset();
            return false;
        });
}

void Input::UpdateState()
{
    std::lock_guard<std::mutex> lock(s_InputMutex);

    s_KeysPressedThisFrame.fill(false);
    s_KeysReleasedThisFrame.fill(false);
    s_MouseButtonsPressedThisFrame.fill(false);
    s_MouseButtonsReleasedThisFrame.fill(false);
    s_MouseDelta = {0, 0};
    s_MouseWheelMove = 0.0f;
}

} // namespace CHEngine
