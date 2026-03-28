#include "input.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include <GLFW/glfw3.h>

namespace CHEngine
{
void Input::PollEvents()
{
    // GLFW events are handled via callbacks or polling.
    // glfwPollEvents() is called in WindowsWindow::EndFrame().
    // We can still use this method to check for specific state if needed,
    // but most events should now come from GLFW callbacks.
}

bool Input::IsKeyPressed(KeyCode key)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, key);
    return state == GLFW_PRESS;
}

bool Input::IsKeyDown(KeyCode key)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsKeyReleased(KeyCode key)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, key);
    return state == GLFW_RELEASE;
}

bool Input::IsKeyUp(KeyCode key)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, key);
    return state == GLFW_RELEASE;
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

bool Input::IsMouseButtonDown(MouseCode button)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

bool Input::IsMouseButtonReleased(MouseCode button)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_RELEASE;
}

bool Input::IsMouseButtonUp(MouseCode button)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_RELEASE;
}

Vector2 Input::GetMousePosition()
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return { (float)xpos, (float)ypos };
}

Vector2 Input::GetMouseDelta()
{
    // GLFW doesn't provide delta directly, we'd need to track it manually
    static Vector2 lastPos = { 0, 0 };
    Vector2 currentPos = GetMousePosition();
    Vector2 delta = { currentPos.x - lastPos.x, currentPos.y - lastPos.y };
    lastPos = currentPos;
    return delta;
}

float Input::GetMouseWheelMove()
{
    // Handled via scroll callback usually
    return 0.0f; 
}
} // namespace CHEngine
