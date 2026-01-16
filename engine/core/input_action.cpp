#include "input_action.h"
#include "engine/core/log.h"

namespace CHEngine
{

InputAction::InputAction(const std::string &name, InputActionType type) : m_Name(name), m_Type(type)
{
}

void InputAction::OnPressed()
{
    for (auto &callback : m_PressedCallbacks)
    {
        callback();
    }
}

void InputAction::OnReleased()
{
    for (auto &callback : m_ReleasedCallbacks)
    {
        callback();
    }
}

void InputAction::OnAxis1D(float value)
{
    for (auto &callback : m_Axis1DCallbacks)
    {
        callback(value);
    }
}

void InputAction::OnAxis2D(Vector2 value)
{
    for (auto &callback : m_Axis2DCallbacks)
    {
        callback(value);
    }
}

void InputAction::SubscribePressed(std::function<void()> callback)
{
    if (m_Type != InputActionType::Button)
    {
        CH_CORE_WARN("Trying to subscribe to Pressed event on non-Button action: {0}", m_Name);
        return;
    }
    m_PressedCallbacks.push_back(callback);
}

void InputAction::SubscribeReleased(std::function<void()> callback)
{
    if (m_Type != InputActionType::Button)
    {
        CH_CORE_WARN("Trying to subscribe to Released event on non-Button action: {0}", m_Name);
        return;
    }
    m_ReleasedCallbacks.push_back(callback);
}

void InputAction::SubscribeAxis1D(std::function<void(float)> callback)
{
    if (m_Type != InputActionType::Axis1D)
    {
        CH_CORE_WARN("Trying to subscribe to Axis1D event on non-Axis1D action: {0}", m_Name);
        return;
    }
    m_Axis1DCallbacks.push_back(callback);
}

void InputAction::SubscribeAxis2D(std::function<void(Vector2)> callback)
{
    if (m_Type != InputActionType::Axis2D)
    {
        CH_CORE_WARN("Trying to subscribe to Axis2D event on non-Axis2D action: {0}", m_Name);
        return;
    }
    m_Axis2DCallbacks.push_back(callback);
}

void InputAction::ClearSubscribers()
{
    m_PressedCallbacks.clear();
    m_ReleasedCallbacks.clear();
    m_Axis1DCallbacks.clear();
    m_Axis2DCallbacks.clear();
}

} // namespace CHEngine
