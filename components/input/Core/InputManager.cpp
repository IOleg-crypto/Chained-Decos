#include "InputManager.h"
#include <raylib.h>

bool InputManager::Initialize()
{
    TraceLog(LOG_INFO, "InputManager initialized");
    m_initialized = true;
    return true;
}

void InputManager::Shutdown()
{
    ClearActions();
    m_initialized = false;
    TraceLog(LOG_INFO, "InputManager shutdown");
}

void InputManager::Update(float deltaTime)
{
    // Update mouse delta
    Vector2 currentMousePos = GetMousePosition();
    m_lastMousePosition = currentMousePos;
}

void InputManager::RegisterAction(int key, const std::function<void()> &action, InputType type)
{
    switch (type)
    {
    case InputType::PRESSED:
        RegisterPressedAction(key, action);
        break;
    case InputType::HELD:
        RegisterHeldAction(key, action);
        break;
    case InputType::RELEASED:
        RegisterReleasedAction(key, action);
        break;
    }
}

void InputManager::RegisterPressedAction(int key, const std::function<void()> &action)
{
    m_pressedActions[key] = action;
}

void InputManager::RegisterHeldAction(int key, const std::function<void()> &action)
{
    m_heldActions[key] = action;
}

void InputManager::RegisterReleasedAction(int key, const std::function<void()> &action)
{
    m_releasedActions[key] = action;
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
    // Process pressed actions
    for (const auto &[key, action] : m_pressedActions)
    {
        if (IsKeyPressed(key))
        {
            action();
        }
    }

    // Process held actions
    for (const auto &[key, action] : m_heldActions)
    {
        if (IsKeyDown(key))
        {
            action();
        }
    }

    // Process released actions
    for (const auto &[key, action] : m_releasedActions)
    {
        if (IsKeyReleased(key))
        {
            action();
        }
    }
}

bool InputManager::IsKeyPressed(int key) const
{
    return ::IsKeyPressed(key);
}

bool InputManager::IsKeyDown(int key) const
{
    return ::IsKeyDown(key);
}

bool InputManager::IsKeyReleased(int key) const
{
    return ::IsKeyReleased(key);
}

Vector2 InputManager::GetMousePosition() const
{
    return ::GetMousePosition();
}

Vector2 InputManager::GetMouseDelta() const
{
    return ::GetMouseDelta();
}

bool InputManager::IsMouseButtonPressed(int button) const
{
    return ::IsMouseButtonPressed(button);
}

bool InputManager::IsMouseButtonDown(int button) const
{
    return ::IsMouseButtonDown(button);
}

bool InputManager::IsMouseButtonReleased(int button) const
{
    return ::IsMouseButtonReleased(button);
}

void InputManager::DisableCursor()
{
    ::DisableCursor();
}

void InputManager::EnableCursor()
{
    ::EnableCursor();
}

bool InputManager::IsCursorDisabled() const
{
    return ::IsCursorHidden();
}
