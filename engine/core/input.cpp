#include "engine/core/input.h"
#include "engine/core/ch_assert.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace CHEngine
{
    Input* Input::s_Instance = nullptr;

    Input::Input()
    {
        CH_CORE_ASSERT(!s_Instance, "Input already exists!");
        s_Instance = this;

        memset(m_KeyStates, 0, sizeof(m_KeyStates));
        memset(m_LastKeyStates, 0, sizeof(m_LastKeyStates));
        memset(m_MouseStates, 0, sizeof(m_MouseStates));
        memset(m_LastMouseStates, 0, sizeof(m_LastMouseStates));
        
        m_MousePosition = { 0.0f, 0.0f };
        m_LastMousePosition = { 0.0f, 0.0f };
        m_MouseWheelDelta = 0.0f;
        m_CurrentMouseWheelDelta = 0.0f;
    }

    Input::~Input()
    {
        s_Instance = nullptr;
    }

    void Input::OnInit()
    {
    }

    void Input::OnUpdate(Timestep ts)
    {
        for (int i = 0; i < 512; i++)
            m_LastKeyStates[i] = m_KeyStates[i];

        for (int i = 0; i < 16; i++)
            m_LastMouseStates[i] = m_MouseStates[i];

        m_LastMousePosition = m_MousePosition;
        m_CurrentMouseWheelDelta = m_MouseWheelDelta;
        m_MouseWheelDelta = 0.0f;
        
        glfwPollEvents();
    }

    void Input::OnShutdown()
    {
    }

    bool Input::IsKeyPressedImpl(KeyCode key) const
    {
        return m_KeyStates[(int)key] && !m_LastKeyStates[(int)key];
    }

    bool Input::IsKeyDownImpl(KeyCode key) const
    {
        return m_KeyStates[(int)key];
    }

    bool Input::IsKeyReleasedImpl(KeyCode key) const
    {
        return !m_KeyStates[(int)key] && m_LastKeyStates[(int)key];
    }

    bool Input::IsKeyUpImpl(KeyCode key) const
    {
        return !m_KeyStates[(int)key];
    }

    bool Input::IsMouseButtonPressedImpl(MouseCode button) const
    {
        return m_MouseStates[(int)button] && !m_LastMouseStates[(int)button];
    }

    bool Input::IsMouseButtonDownImpl(MouseCode button) const
    {
        return m_MouseStates[(int)button];
    }

    bool Input::IsMouseButtonReleasedImpl(MouseCode button) const
    {
        return !m_MouseStates[(int)button] && m_LastMouseStates[(int)button];
    }

    bool Input::IsMouseButtonUpImpl(MouseCode button) const
    {
        return !m_MouseStates[(int)button];
    }

    glm::vec2 Input::GetMousePositionImpl() const
    {
        return m_MousePosition;
    }

    glm::vec2 Input::GetMouseDeltaImpl() const
    {
        return m_MousePosition - m_LastMousePosition;
    }

    float Input::GetMouseWheelMoveImpl() const
    {
        return m_CurrentMouseWheelDelta;
    }
}
