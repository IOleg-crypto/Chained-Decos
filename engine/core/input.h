#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/core/base.h"
#include "key_codes.h"
#include "mouse_codes.h"

namespace CHEngine
{

class Input
{
public:
    static void Update();
    static void PollEvents();

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

    // Callbacks used by Window to update states
    static void OnKey(int key, bool pressed) { if (key >= 0 && key < 512) s_KeyStates[key] = pressed; }
    static void OnMouseButton(int button, bool pressed) { if (button >= 0 && button < 16) s_MouseStates[button] = pressed; }
    static void OnMouseMove(float x, float y) { s_MousePosition = { x, y }; }
    static void OnMouseScroll(float xOffset, float yOffset) { s_MouseWheelDelta = yOffset; }

private:
    static bool s_KeyStates[512];
    static bool s_LastKeyStates[512];
    static bool s_MouseStates[16];
    static bool s_LastMouseStates[16];

    static glm::vec2 s_MousePosition;
    static glm::vec2 s_LastMousePosition;
    static float s_MouseWheelDelta;
    static float s_CurrentMouseWheelDelta;
};

} // namespace CHEngine

#endif // CH_INPUT_H
