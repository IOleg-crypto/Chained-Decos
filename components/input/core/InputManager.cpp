#include "InputManager.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include <raylib.h>

using namespace ChainedDecos;

InputManager::InputManager() = default;

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
    if (!m_EventCallback)
        return;

    // We can't easily iterate all keys in Raylib without a loop or knowing which ones were pressed.
    // However, for typical engine architecture (The Cherno style), we usually get events from the
    // windowing system (GLFW). Raylib is a bit higher level. Let's poll common keys or use a hybrid
    // approach. Actually, Chained Decos uses Raylib's IsKeyPressed etc.

    // For a cleaner approach, we might want to poll all keys from 32 to 348 (GLFW range) or
    // similar.
    for (int key = 32; key < 348; key++)
    {
        if (IsKeyPressed(key))
        {
            KeyPressedEvent e(key, 0);
            m_EventCallback(e);
        }
        if (IsKeyReleased(key))
        {
            KeyReleasedEvent e(key);
            m_EventCallback(e);
        }
    }

    // Mouse events
    Vector2 mousePos = GetMousePosition();
    static Vector2 lastPos = mousePos;
    if (mousePos.x != lastPos.x || mousePos.y != lastPos.y)
    {
        MouseMovedEvent e(mousePos.x, mousePos.y);
        m_EventCallback(e);
        lastPos = mousePos;
    }

    for (int button = 0; button < 3; button++)
    {
        if (IsMouseButtonPressed(button))
        {
            MouseButtonPressedEvent e(button);
            m_EventCallback(e);
        }
        if (IsMouseButtonReleased(button))
        {
            MouseButtonReleasedEvent e(button);
            m_EventCallback(e);
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        m_EventCallback(e);
    }

    // Process legacy actions
    for (const auto &[key, action] : m_pressedActions)
    {
        if (IsKeyPressed(key))
        {
            action();
        }
    }

    for (const auto &[key, action] : m_heldActions)
    {
        if (IsKeyDown(key))
        {
            action();
        }
    }

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

float InputManager::GetMouseWheelMove() const
{
    return ::GetMouseWheelMove();
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


