#include "input.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include "raylib.h"

namespace CHEngine
{
void Input::PollEvents()
{
    // 1. Keyboard Events
    int key;
    while ((key = GetKeyPressed()) != 0)
    {
        KeyPressedEvent e(key, false);
        Application::Get().OnEvent(e);
    }

    // 1.5 Window Events
    if (::IsWindowResized())
    {
        WindowResizeEvent e(::GetScreenWidth(), ::GetScreenHeight());
        Application::Get().OnEvent(e);
    }

    // 2. Mouse Buttons
    auto handleMouse = [&](int button) {
        if (::IsMouseButtonPressed(button))
        {
            MouseButtonPressedEvent e(button);
            Application::Get().OnEvent(e);
        }
        if (::IsMouseButtonReleased(button))
        {
            MouseButtonReleasedEvent e(button);
            Application::Get().OnEvent(e);
        }
    };

    handleMouse(MOUSE_BUTTON_LEFT);
    handleMouse(MOUSE_BUTTON_RIGHT);
    handleMouse(MOUSE_BUTTON_MIDDLE);

    // 3. Mouse Wheel
    float wheel = ::GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        Application::Get().OnEvent(e);
    }
}

bool Input::IsKeyPressed(KeyCode key)
{
    return ::IsKeyPressed(key);
}
bool Input::IsKeyDown(KeyCode key)
{
    bool down = ::IsKeyDown(key);
    // if (down) CH_CORE_INFO("Input::IsKeyDown(key={}) -> true", key);
    return down;
}
bool Input::IsKeyReleased(KeyCode key)
{
    return ::IsKeyReleased(key);
}
bool Input::IsKeyUp(KeyCode key)
{
    return ::IsKeyUp(key);
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    return ::IsMouseButtonPressed(button);
}
bool Input::IsMouseButtonDown(MouseCode button)
{
    return ::IsMouseButtonDown(button);
}
bool Input::IsMouseButtonReleased(MouseCode button)
{
    return ::IsMouseButtonReleased(button);
}
bool Input::IsMouseButtonUp(MouseCode button)
{
    return ::IsMouseButtonUp(button);
}

Vector2 Input::GetMousePosition()
{
    return ::GetMousePosition();
}
Vector2 Input::GetMouseDelta()
{
    return ::GetMouseDelta();
}
float Input::GetMouseWheelMove()
{
    return ::GetMouseWheelMove();
}
} // namespace CHEngine
