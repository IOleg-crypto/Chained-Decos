#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/common/base.h"
#include "engine/common/timestep.h"
#include "engine/core/engine_module.h"
#include "key_codes.h"
#include "mouse_codes.h"
#include <array>
#include <glm/vec2.hpp>

namespace Chained::Core
{
class CH_API Input : public EngineModule
{
public:
    Input();
    ~Input() override;

    void Initialize() override;
    void Shutdown() override;

    void UpdateImpl(Timestep ts);
    void ResetAllImpl();

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

    void OnKeyImpl(KeyCode key, bool pressed);
    void OnMouseButtonImpl(MouseCode button, bool pressed);
    void OnMouseMoveImpl(float x, float y);
    void OnMouseScrollImpl(float xOffset, float yOffset);

    // Static API Facade for convenient engine-wide access
    static void Init();
    static void ShutdownStatic();
    static void Update(Timestep ts);
    static void ResetAll();

    static bool IsKeyPressed(KeyCode key);
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyReleased(KeyCode key);
    static bool IsKeyUp(KeyCode key);

    static bool IsMouseButtonPressed(MouseCode button);
    static bool IsMouseButtonDown(MouseCode button);
    static bool IsMouseButtonReleased(MouseCode button);
    static bool IsMouseButtonUp(MouseCode button);

    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static float GetMouseWheelMove();

    static void OnKey(KeyCode key, bool pressed);
    static void OnMouseButton(MouseCode button, bool pressed);
    static void OnMouseMove(float x, float y);
    static void OnMouseScroll(float xOffset, float yOffset);

    static Input* GetInstance()
    {
        return s_Instance;
    }

private:
    static Input* s_Instance;

    std::array<bool, 512> m_KeyStates{};
    std::array<bool, 512> m_LastKeyStates{};
    std::array<bool, 16> m_MouseStates{};
    std::array<bool, 16> m_LastMouseStates{};

    glm::vec2 m_MousePosition{0.0f, 0.0f};
    glm::vec2 m_LastMousePosition{0.0f, 0.0f};
    float m_MouseWheelAccumulator = 0.0f;
    float m_CurrentMouseWheelDelta = 0.0f;
    bool m_FirstMouseUpdate = true;
};
} // namespace Chained::Core

#endif // CH_INPUT_H