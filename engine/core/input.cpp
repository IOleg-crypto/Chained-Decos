#include "engine/core/input.h"
#include <GLFW/glfw3.h>

namespace CHEngine
{
    bool Input::s_KeyStates[512] = { false };
    bool Input::s_LastKeyStates[512] = { false };
    bool Input::s_MouseStates[16] = { false };
    bool Input::s_LastMouseStates[16] = { false };

    glm::vec2 Input::s_MousePosition = { 0.0f, 0.0f };
    glm::vec2 Input::s_LastMousePosition = { 0.0f, 0.0f };
    float Input::s_MouseWheelDelta = 0.0f;
    float Input::s_CurrentMouseWheelDelta = 0.0f;

    void Input::Update()
    {
        for (int i = 0; i < 512; i++)
            s_LastKeyStates[i] = s_KeyStates[i];

        for (int i = 0; i < 16; i++)
            s_LastMouseStates[i] = s_MouseStates[i];

        s_LastMousePosition = s_MousePosition;
        s_CurrentMouseWheelDelta = s_MouseWheelDelta;
        s_MouseWheelDelta = 0.0f;
    }

    void Input::PollEvents()
    {
        glfwPollEvents();
    }

    bool Input::IsKeyPressed(KeyCode key)
    {
        return s_KeyStates[(int)key] && !s_LastKeyStates[(int)key];
    }

    bool Input::IsKeyDown(KeyCode key)
    {
        return s_KeyStates[(int)key];
    }

    bool Input::IsKeyReleased(KeyCode key)
    {
        return !s_KeyStates[(int)key] && s_LastKeyStates[(int)key];
    }

    bool Input::IsKeyUp(KeyCode key)
    {
        return !s_KeyStates[(int)key];
    }

    bool Input::IsMouseButtonPressed(MouseCode button)
    {
        return s_MouseStates[(int)button] && !s_LastMouseStates[(int)button];
    }

    bool Input::IsMouseButtonDown(MouseCode button)
    {
        return s_MouseStates[(int)button];
    }

    bool Input::IsMouseButtonReleased(MouseCode button)
    {
        return !s_MouseStates[(int)button] && s_LastMouseStates[(int)button];
    }

    bool Input::IsMouseButtonUp(MouseCode button)
    {
        return !s_MouseStates[(int)button];
    }

    glm::vec2 Input::GetMousePosition()
    {
        return s_MousePosition;
    }

    glm::vec2 Input::GetMouseDelta()
    {
        return s_MousePosition - s_LastMousePosition;
    }

    float Input::GetMouseWheelMove()
    {
        return s_CurrentMouseWheelDelta;
    }
}
