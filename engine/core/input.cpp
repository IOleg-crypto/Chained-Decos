#include "input.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include <GLFW/glfw3.h>

namespace CHEngine
{
void Input::Update()
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    if (!window) return;

    // 1. Sync Key States
    // GLFW_KEY_SPACE (32) is the first valid named key; indices 0..31 are reserved and
    // trigger GLFW's error callback if polled. Poll only the valid range.
    for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++)
    {
        Input::s_LastKeyStates[i] = Input::s_KeyStates[i];
        Input::s_KeyStates[i] = glfwGetKey(window, i) == GLFW_PRESS;
    }

    // 2. Sync Mouse States (GLFW_MOUSE_BUTTON_LAST = 7)
    for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++)
    {
        Input::s_LastMouseStates[i] = Input::s_MouseStates[i];
        Input::s_MouseStates[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
    }

    // 3. Sync Mouse Position
    Input::s_LastMousePosition = Input::s_MousePosition;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    Input::s_MousePosition = { (float)xpos, (float)ypos };

    // 4. Sync Mouse Wheel (Impulse)
    Input::s_CurrentMouseWheelDelta = Input::s_MouseWheelDelta;
    Input::s_MouseWheelDelta = 0.0f;
}

void Input::PollEvents()
{
}

bool Input::IsKeyPressed(KeyCode key)
{
    return Input::s_KeyStates[key] && !Input::s_LastKeyStates[key];
}

bool Input::IsKeyDown(KeyCode key)
{
    return Input::s_KeyStates[key];
}

bool Input::IsKeyReleased(KeyCode key)
{
    return !Input::s_KeyStates[key] && Input::s_LastKeyStates[key];
}

bool Input::IsKeyUp(KeyCode key)
{
    return !Input::s_KeyStates[key];
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    return Input::s_MouseStates[button] && !Input::s_LastMouseStates[button];
}

bool Input::IsMouseButtonDown(MouseCode button)
{
    return Input::s_MouseStates[button];
}

bool Input::IsMouseButtonReleased(MouseCode button)
{
    return !Input::s_MouseStates[button] && Input::s_LastMouseStates[button];
}

bool Input::IsMouseButtonUp(MouseCode button)
{
    return !Input::s_MouseStates[button];
}

glm::vec2 Input::GetMousePosition()
{
    return Input::s_MousePosition;
}

glm::vec2 Input::GetMouseDelta()
{
    return Input::s_MousePosition - Input::s_LastMousePosition;
}

float Input::GetMouseWheelMove()
{
    return Input::s_CurrentMouseWheelDelta; 
}
} // namespace CHEngine
