#include "input.h"

namespace CHEngine
{
void Input::Update()
{
}
bool Input::IsKeyPressed(int keycode)
{
    return IsKeyDown(keycode);
}
bool Input::IsMouseButtonPressed(int button)
{
    return IsMouseButtonDown(button);
}
Vector2 Input::GetMousePosition()
{
    return ::GetMousePosition();
}
float Input::GetMouseX()
{
    return ::GetMousePosition().x;
}
float Input::GetMouseY()
{
    return ::GetMousePosition().y;
}
} // namespace CHEngine
