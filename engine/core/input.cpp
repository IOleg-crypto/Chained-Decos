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

        // 2. Mouse Events
        auto handleMouse = [&](int button)
        {
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

        Vector2 currentMousePos = ::GetMousePosition();
        static Vector2 lastMousePos = {0, 0};
        if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y)
        {
            MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
            Application::Get().OnEvent(e);
            lastMousePos = currentMousePos;
        }

        float wheel = ::GetMouseWheelMove();
        if (wheel != 0)
        {
            MouseScrolledEvent e(0, wheel);
            Application::Get().OnEvent(e);
        }
    }

    bool Input::IsKeyPressed(int key) { return ::IsKeyPressed(key); }
    bool Input::IsKeyDown(int key) { 
        bool down = ::IsKeyDown(key);
        if (down) CH_CORE_INFO("Input::IsKeyDown(key={}) -> true", key);
        return down; 
    }
    bool Input::IsKeyReleased(int key) { return ::IsKeyReleased(key); }
    bool Input::IsKeyUp(int key) { return ::IsKeyUp(key); }

    bool Input::IsMouseButtonPressed(int button) { return ::IsMouseButtonPressed(button); }
    bool Input::IsMouseButtonDown(int button) { return ::IsMouseButtonDown(button); }
    bool Input::IsMouseButtonReleased(int button) { return ::IsMouseButtonReleased(button); }
    bool Input::IsMouseButtonUp(int button) { return ::IsMouseButtonUp(button); }

    Vector2 Input::GetMousePosition() { return ::GetMousePosition(); }
    Vector2 Input::GetMouseDelta() { return ::GetMouseDelta(); }
    float Input::GetMouseWheelMove() { return ::GetMouseWheelMove(); }
}
