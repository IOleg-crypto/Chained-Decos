#include "Input.h"

namespace CHEngine
{
std::map<int, std::function<void()>> Input::s_Actions;

void Input::Update()
{
    for (auto const &[key, action] : s_Actions)
    {
        if (::IsKeyPressed(key))
        {
            action();
        }
    }
}

void Input::RegisterAction(int key, const std::function<void()> &action)
{
    s_Actions[key] = action;
}

} // namespace CHEngine
