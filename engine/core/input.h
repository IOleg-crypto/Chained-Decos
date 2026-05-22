#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/core/base.h"
#include "engine/core/engine_service.h"
#include "engine/core/timestep.h"
#include "key_codes.h"
#include "mouse_codes.h"

namespace CHEngine
{

class Input : public EngineService
{
public:
    Input();
    virtual ~Input() override;

    static bool IsKeyPressed(KeyCode key) { return s_Instance->IsKeyPressedImpl(key); }
    static bool IsKeyDown(KeyCode key) { return s_Instance->IsKeyDownImpl(key); }
    static bool IsKeyReleased(KeyCode key) { return s_Instance->IsKeyReleasedImpl(key); }
    static bool IsKeyUp(KeyCode key) { return s_Instance->IsKeyUpImpl(key); }

    static bool IsMouseButtonPressed(MouseCode button) { return s_Instance->IsMouseButtonPressedImpl(button); }
    static bool IsMouseButtonDown(MouseCode button) { return s_Instance->IsMouseButtonDownImpl(button); }
    static bool IsMouseButtonReleased(MouseCode button) { return s_Instance->IsMouseButtonReleasedImpl(button); }
    static bool IsMouseButtonUp(MouseCode button) { return s_Instance->IsMouseButtonUpImpl(button); }

    static glm::vec2 GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
    static glm::vec2 GetMouseDelta() { return s_Instance->GetMouseDeltaImpl(); }
    static float GetMouseWheelMove() { return s_Instance->GetMouseWheelMoveImpl(); }

    // Callbacks used by Window to update states
    static void OnKey(int key, bool pressed) { if (key >= 0 && key < 512) s_Instance->m_KeyStates[key] = pressed; }
    static void OnMouseButton(int button, bool pressed) { if (button >= 0 && button < 16) s_Instance->m_MouseStates[button] = pressed; }
    static void OnMouseMove(float x, float y) { s_Instance->m_MousePosition = { x, y }; }
    static void OnMouseScroll(float xOffset, float yOffset) { s_Instance->m_MouseWheelDelta = yOffset; }

protected:
    virtual void OnInit() override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnShutdown() override;

private:
    bool IsKeyPressedImpl(KeyCode key) const;
    bool IsKeyDownImpl(KeyCode key) const;
    bool IsKeyReleasedImpl(KeyCode key) const;
    bool IsKeyUpImpl(KeyCode key) const;

    bool IsMouseButtonPressedImpl(MouseCode button) const;
    bool IsMouseButtonDownImpl(MouseCode button) const;
    bool IsMouseButtonReleasedImpl(MouseCode button) const;
    bool IsMouseButtonUpImpl(MouseCode button) const;

    glm::vec2 GetMousePositionImpl() const;
    glm::vec2 GetMouseDeltaImpl() const;
    float GetMouseWheelMoveImpl() const;

private:
    bool m_KeyStates[512];
    bool m_LastKeyStates[512];
    bool m_MouseStates[16];
    bool m_LastMouseStates[16];

    glm::vec2 m_MousePosition;
    glm::vec2 m_LastMousePosition;
    float m_MouseWheelDelta;
    float m_CurrentMouseWheelDelta;

    static Input* s_Instance;
};

} // namespace CHEngine

#endif // CH_INPUT_H
