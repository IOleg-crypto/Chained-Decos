#include "input.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"

namespace Chained::Core
{
Input* Input::s_Instance = nullptr;

Input::Input()
{
    if (!s_Instance)
    {
        s_Instance = this;
    }
}

Input::~Input()
{
    if (s_Instance == this)
    {
        s_Instance = nullptr;
    }
}

void Input::Initialize()
{
    ResetAllImpl();
}

void Input::Shutdown()
{
    ResetAllImpl();
}

void Input::ResetAllImpl()
{
    m_KeyStates.fill(false);
    m_LastKeyStates.fill(false);
    m_MouseStates.fill(false);
    m_LastMouseStates.fill(false);
    m_MousePosition = {0.0f, 0.0f};
    m_LastMousePosition = {0.0f, 0.0f};
    m_MouseWheelAccumulator = 0.0f;
    m_CurrentMouseWheelDelta = 0.0f;
    m_FirstMouseUpdate = true;
}

void Input::UpdateImpl(Timestep ts)
{
    m_LastKeyStates = m_KeyStates;
    m_LastMouseStates = m_MouseStates;

    m_LastMousePosition = m_MousePosition;

    m_CurrentMouseWheelDelta = m_MouseWheelAccumulator;
    m_MouseWheelAccumulator = 0.0f;
}

bool Input::IsKeyPressedImpl(KeyCode key) const
{
    auto code = static_cast<size_t>(key);
    if (code >= m_KeyStates.size())
    {
        return false;
    }
    return m_KeyStates[code] && !m_LastKeyStates[code];
}

bool Input::IsKeyDownImpl(KeyCode key) const
{
    auto code = static_cast<size_t>(key);
    if (code >= m_KeyStates.size())
    {
        return false;
    }
    return m_KeyStates[code];
}

bool Input::IsKeyReleasedImpl(KeyCode key) const
{
    auto code = static_cast<size_t>(key);
    if (code >= m_KeyStates.size())
    {
        return false;
    }
    return !m_KeyStates[code] && m_LastKeyStates[code];
}

bool Input::IsKeyUpImpl(KeyCode key) const
{
    auto code = static_cast<size_t>(key);
    if (code >= m_KeyStates.size())
    {
        return true;
    }
    return !m_KeyStates[code];
}

bool Input::IsMouseButtonPressedImpl(MouseCode button) const
{
    auto code = static_cast<size_t>(button);
    if (code >= m_MouseStates.size())
    {
        return false;
    }
    return m_MouseStates[code] && !m_LastMouseStates[code];
}

bool Input::IsMouseButtonDownImpl(MouseCode button) const
{
    auto code = static_cast<size_t>(button);
    if (code >= m_MouseStates.size())
    {
        return false;
    }
    return m_MouseStates[code];
}

bool Input::IsMouseButtonReleasedImpl(MouseCode button) const
{
    auto code = static_cast<size_t>(button);
    if (code >= m_MouseStates.size())
    {
        return false;
    }
    return !m_MouseStates[code] && m_LastMouseStates[code];
}

bool Input::IsMouseButtonUpImpl(MouseCode button) const
{
    auto code = static_cast<size_t>(button);
    if (code >= m_MouseStates.size())
    {
        return true;
    }
    return !m_MouseStates[code];
}

glm::vec2 Input::GetMousePositionImpl() const
{
    return m_MousePosition;
}

glm::vec2 Input::GetMouseDeltaImpl() const
{
    if (m_FirstMouseUpdate)
    {
        return {0.0f, 0.0f};
    }
    return m_MousePosition - m_LastMousePosition;
}

float Input::GetMouseWheelMoveImpl() const
{
    return m_CurrentMouseWheelDelta;
}

void Input::OnKeyImpl(KeyCode key, bool pressed)
{
    auto code = static_cast<size_t>(key);
    if (code < m_KeyStates.size())
    {
        m_KeyStates[code] = pressed;
    }
}

void Input::OnMouseButtonImpl(MouseCode button, bool pressed)
{
    auto code = static_cast<size_t>(button);
    if (code < m_MouseStates.size())
    {
        m_MouseStates[code] = pressed;
    }
}

void Input::OnMouseMoveImpl(float x, float y)
{
    if (m_FirstMouseUpdate)
    {
        m_LastMousePosition = {x, y};
        m_FirstMouseUpdate = false;
    }
    m_MousePosition = {x, y};
}

void Input::OnMouseScrollImpl(float xOffset, float yOffset)
{
    m_MouseWheelAccumulator += yOffset;
}

// Static API Facade implementations
void Input::Init()
{
    if (!s_Instance)
    {
        auto instance = ServiceLocator::TryGet<Input>();
        if (!instance)
        {
            static Input localInstance;
            s_Instance = &localInstance;
        }
    }
    if (s_Instance)
    {
        s_Instance->Initialize();
    }
}

void Input::ShutdownStatic()
{
    if (s_Instance)
    {
        s_Instance->Shutdown();
    }
}

void Input::Update(Timestep ts)
{
    if (s_Instance)
    {
        s_Instance->UpdateImpl(ts);
    }
}

void Input::ResetAll()
{
    if (s_Instance)
    {
        s_Instance->ResetAllImpl();
    }
}

bool Input::IsKeyPressed(KeyCode key)
{
    return s_Instance ? s_Instance->IsKeyPressedImpl(key) : false;
}

bool Input::IsKeyDown(KeyCode key)
{
    return s_Instance ? s_Instance->IsKeyDownImpl(key) : false;
}

bool Input::IsKeyReleased(KeyCode key)
{
    return s_Instance ? s_Instance->IsKeyReleasedImpl(key) : false;
}

bool Input::IsKeyUp(KeyCode key)
{
    return s_Instance ? s_Instance->IsKeyUpImpl(key) : true;
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    return s_Instance ? s_Instance->IsMouseButtonPressedImpl(button) : false;
}

bool Input::IsMouseButtonDown(MouseCode button)
{
    return s_Instance ? s_Instance->IsMouseButtonDownImpl(button) : false;
}

bool Input::IsMouseButtonReleased(MouseCode button)
{
    return s_Instance ? s_Instance->IsMouseButtonReleasedImpl(button) : false;
}

bool Input::IsMouseButtonUp(MouseCode button)
{
    return s_Instance ? s_Instance->IsMouseButtonUpImpl(button) : true;
}

glm::vec2 Input::GetMousePosition()
{
    return s_Instance ? s_Instance->GetMousePositionImpl() : glm::vec2{0.0f, 0.0f};
}

glm::vec2 Input::GetMouseDelta()
{
    return s_Instance ? s_Instance->GetMouseDeltaImpl() : glm::vec2{0.0f, 0.0f};
}

float Input::GetMouseWheelMove()
{
    return s_Instance ? s_Instance->GetMouseWheelMoveImpl() : 0.0f;
}

void Input::OnKey(KeyCode key, bool pressed)
{
    if (s_Instance)
    {
        s_Instance->OnKeyImpl(key, pressed);
    }
}

void Input::OnMouseButton(MouseCode button, bool pressed)
{
    if (s_Instance)
    {
        s_Instance->OnMouseButtonImpl(button, pressed);
    }
}

void Input::OnMouseMove(float x, float y)
{
    if (s_Instance)
    {
        s_Instance->OnMouseMoveImpl(x, y);
    }
}

void Input::OnMouseScroll(float xOffset, float yOffset)
{
    if (s_Instance)
    {
        s_Instance->OnMouseScrollImpl(xOffset, yOffset);
    }
}

} // namespace Chained::Core