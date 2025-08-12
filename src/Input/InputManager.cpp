//
// Created by I#Oleg.
//

#include "InputManager.h"

void InputManager::RegisterAction(const int key, const std::function<void()> &action,
                                  InputType type)
{
    switch (type)
    {
    case InputType::PRESSED:
        m_pressedActions[key] = action;
        break;
    case InputType::HELD:
        m_heldActions[key] = action;
        break;
    case InputType::RELEASED:
        m_releasedActions[key] = action;
        break;
    }
}

void InputManager::RegisterPressedAction(int key, const std::function<void()> &action)
{
    RegisterAction(key, action, InputType::PRESSED);
}

void InputManager::RegisterHeldAction(int key, const std::function<void()> &action)
{
    RegisterAction(key, action, InputType::HELD);
}

void InputManager::RegisterReleasedAction(int key, const std::function<void()> &action)
{
    RegisterAction(key, action, InputType::RELEASED);
}

void InputManager::UnregisterAction(int key, InputType type)
{
    switch (type)
    {
    case InputType::PRESSED:
        m_pressedActions.erase(key);
        break;
    case InputType::HELD:
        m_heldActions.erase(key);
        break;
    case InputType::RELEASED:
        m_releasedActions.erase(key);
        break;
    }
}

void InputManager::ClearActions()
{
    m_pressedActions.clear();
    m_heldActions.clear();
    m_releasedActions.clear();
}

void InputManager::ProcessInput() const
{
    // Process single press actions
    for (const auto &[key, action] : m_pressedActions)
    {
        if (IsKeyPressed(key))
        {
            action();
        }
    }

    // Process continuous hold actions
    for (const auto &[key, action] : m_heldActions)
    {
        if (IsKeyDown(key))
        {
            action();
        }
    }

    // Process release actions
    for (const auto &[key, action] : m_releasedActions)
    {
        if (IsKeyReleased(key))
        {
            action();
        }
    }
}
