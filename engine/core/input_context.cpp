#include "input_context.h"
#include "engine/core/log.h"
#include "input.h"
#include <algorithm>


namespace CHEngine
{

InputContext::InputContext(const std::string &name) : m_Name(name)
{
}

void InputContext::RegisterAction(const std::string &name, InputActionType type)
{
    if (m_Actions.find(name) != m_Actions.end())
    {
        CH_CORE_WARN("Action '{0}' already registered in context '{1}'", name, m_Name);
        return;
    }

    m_Actions[name] = std::make_shared<InputAction>(name, type);
    CH_CORE_INFO("Registered action '{0}' in context '{1}'", name, m_Name);
}

InputAction *InputContext::GetAction(const std::string &name)
{
    auto it = m_Actions.find(name);
    if (it != m_Actions.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void InputContext::AddBinding(const InputBinding &binding)
{
    // Verify action exists
    if (m_Actions.find(binding.ActionName) == m_Actions.end())
    {
        CH_CORE_ERROR("Cannot add binding: Action '{0}' not found in context '{1}'",
                      binding.ActionName, m_Name);
        return;
    }

    m_Bindings.push_back(binding);
}

void InputContext::RemoveBinding(const std::string &actionName, int keyCode)
{
    m_Bindings.erase(std::remove_if(m_Bindings.begin(), m_Bindings.end(), [&](const InputBinding &b)
                                    { return b.ActionName == actionName && b.KeyCode == keyCode; }),
                     m_Bindings.end());
}

void InputContext::ClearBindings()
{
    m_Bindings.clear();
}

std::vector<InputBinding> InputContext::GetBindingsForAction(const std::string &actionName) const
{
    std::vector<InputBinding> result;
    for (const auto &binding : m_Bindings)
    {
        if (binding.ActionName == actionName)
        {
            result.push_back(binding);
        }
    }
    return result;
}

void InputContext::ProcessKeyPressed(int keyCode)
{
    for (const auto &binding : m_Bindings)
    {
        if (binding.KeyCode == keyCode && binding.ModifiersMatch())
        {
            auto *action = GetAction(binding.ActionName);
            if (action && action->GetType() == InputActionType::Button)
            {
                action->OnPressed();
            }
        }
    }
}

void InputContext::ProcessKeyReleased(int keyCode)
{
    for (const auto &binding : m_Bindings)
    {
        if (binding.KeyCode == keyCode && binding.ModifiersMatch())
        {
            auto *action = GetAction(binding.ActionName);
            if (action && action->GetType() == InputActionType::Button)
            {
                action->OnReleased();
            }
        }
    }
}

void InputContext::ProcessAxisInput()
{
    // Reset accumulators
    m_AxisAccumulators.clear();

    // Accumulate axis values from all active bindings
    for (const auto &binding : m_Bindings)
    {
        if (Input::IsKeyDown(binding.KeyCode) && binding.ModifiersMatch())
        {
            auto *action = GetAction(binding.ActionName);
            if (!action)
                continue;

            if (action->GetType() == InputActionType::Axis2D)
            {
                Vector2 &accumulator = m_AxisAccumulators[binding.ActionName];

                if (binding.Axis == InputAxis::X)
                {
                    accumulator.x += binding.Scale;
                }
                else if (binding.Axis == InputAxis::Y)
                {
                    accumulator.y += binding.Scale;
                }
            }
            else if (action->GetType() == InputActionType::Axis1D)
            {
                action->OnAxis1D(binding.Scale);
            }
        }
    }

    // Trigger Axis2D actions with accumulated values
    for (const auto &[actionName, value] : m_AxisAccumulators)
    {
        auto *action = GetAction(actionName);
        if (action)
        {
            action->OnAxis2D(value);
        }
    }
}

} // namespace CHEngine
