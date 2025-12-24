#include "Input.h"
#include "components/input/interfaces/IInputManager.h"
#include "core/Engine.h"


namespace CHEngine
{

void Input::RegisterAction(int key, const std::function<void()> &action)
{
    Engine::Instance().GetInputManager().RegisterAction(key, action);
}

} // namespace CHEngine
