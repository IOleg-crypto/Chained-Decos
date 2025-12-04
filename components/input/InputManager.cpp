#include "InputManager.h"

namespace Servers
{

void InputManager::Update()
{
    // Raylib handles input polling internally
}

bool InputManager::IsPressed(int key)
{
    return IsKeyPressed(key);
}

bool InputManager::IsDown(int key)
{
    return IsKeyDown(key);
}

bool InputManager::IsReleased(int key)
{
    return IsKeyReleased(key);
}

bool InputManager::IsMousePressed(int button)
{
    return ::IsMouseButtonPressed(button);
}

bool InputManager::IsMouseDown(int button)
{
    return ::IsMouseButtonDown(button);
}

bool InputManager::IsMouseReleased(int button)
{
    return ::IsMouseButtonReleased(button);
}

Vector2 InputManager::GetMousePosition()
{
    return ::GetMousePosition();
}

Vector2 InputManager::GetMouseDelta()
{
    return ::GetMouseDelta();
}

float InputManager::GetMouseWheel()
{
    return ::GetMouseWheelMove();
}

} // namespace Servers
