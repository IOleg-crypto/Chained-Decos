#include "input.h"
#include <cstring>

namespace Chained::Core::Input
{
    namespace
    {
        bool s_KeyStates[512]{};
        bool s_LastKeyStates[512]{};
        bool s_MouseStates[16]{};
        bool s_LastMouseStates[16]{};

        glm::vec2 s_MousePosition{};
        glm::vec2 s_LastMousePosition{};
        float s_MouseWheelDelta = 0.0f;
        float s_CurrentMouseWheelDelta = 0.0f;
    }

    void Init()
    {
        std::memset(s_KeyStates, 0, sizeof(s_KeyStates));
        std::memset(s_LastKeyStates, 0, sizeof(s_LastKeyStates));
        std::memset(s_MouseStates, 0, sizeof(s_MouseStates));
        std::memset(s_LastMouseStates, 0, sizeof(s_LastMouseStates));

        s_MousePosition = { 0.0f, 0.0f };
        s_LastMousePosition = { 0.0f, 0.0f };
        s_MouseWheelDelta = 0.0f;
        s_CurrentMouseWheelDelta = 0.0f;
    }

    void Shutdown()
    {
    }

    void ResetAll()
    {
        std::memset(s_KeyStates, 0, sizeof(s_KeyStates));
        std::memset(s_LastKeyStates, 0, sizeof(s_LastKeyStates));
        std::memset(s_MouseStates, 0, sizeof(s_MouseStates));
        std::memset(s_LastMouseStates, 0, sizeof(s_LastMouseStates));
    }

    void Update(Timestep ts)
    {
        std::memcpy(s_LastKeyStates, s_KeyStates, sizeof(s_KeyStates));
        std::memcpy(s_LastMouseStates, s_MouseStates, sizeof(s_MouseStates));

        s_LastMousePosition = s_MousePosition;

        s_CurrentMouseWheelDelta = s_MouseWheelDelta;
        s_MouseWheelDelta = 0.0f;
    }

    bool IsKeyPressed(KeyCode key)
    {
        auto code = static_cast<int>(key);
        if (code < 0 || code >= 512) return false;
        return s_KeyStates[code] && !s_LastKeyStates[code];
    }

    bool IsKeyDown(KeyCode key)
    {
        auto code = static_cast<int>(key);
        if (code < 0 || code >= 512) return false;
        return s_KeyStates[code];
    }

    bool IsKeyReleased(KeyCode key)
    {
        auto code = static_cast<int>(key);
        if (code < 0 || code >= 512) return false;
        return !s_KeyStates[code] && s_LastKeyStates[code];
    }

    bool IsKeyUp(KeyCode key)
    {
        auto code = static_cast<int>(key);
        if (code < 0 || code >= 512) return true;
        return !s_KeyStates[code];
    }

    bool IsMouseButtonPressed(MouseCode button)
    {
        auto code = static_cast<int>(button);
        if (code < 0 || code >= 16) return false;
        return s_MouseStates[code] && !s_LastMouseStates[code];
    }

    bool IsMouseButtonDown(MouseCode button)
    {
        auto code = static_cast<int>(button);
        if (code < 0 || code >= 16) return false;
        return s_MouseStates[code];
    }

    bool IsMouseButtonReleased(MouseCode button)
    {
        auto code = static_cast<int>(button);
        if (code < 0 || code >= 16) return false;
        return !s_MouseStates[code] && s_LastMouseStates[code];
    }

    bool IsMouseButtonUp(MouseCode button)
    {
        auto code = static_cast<int>(button);
        if (code < 0 || code >= 16) return true;
        return !s_MouseStates[code];
    }

    glm::vec2 GetMousePosition()
    {
        return s_MousePosition;
    }

    glm::vec2 GetMouseDelta()
    {
        return s_MousePosition - s_LastMousePosition;
    }

    float GetMouseWheelMove()
    {
        return s_CurrentMouseWheelDelta;
    }

    void OnKey(KeyCode key, bool pressed)
    {
        int code = static_cast<int>(key);
        if (code >= 0 && code < 512)
        {
            s_KeyStates[code] = pressed;
        }
    }

    void OnMouseButton(MouseCode button, bool pressed)
    {
        int code = static_cast<int>(button);
        if (code >= 0 && code < 16)
        {
            s_MouseStates[code] = pressed;
        }
    }

    void OnMouseMove(float x, float y)
    {
        s_MousePosition = { x, y };
    }

    void OnMouseScroll(float xOffset, float yOffset)
    {
        s_MouseWheelDelta = yOffset;
    }
}