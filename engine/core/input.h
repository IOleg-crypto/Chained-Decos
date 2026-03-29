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

    // Callback used by Window to update scroll
    static void OnMouseScroll(float xOffset, float yOffset) { s_MouseWheelDelta = yOffset; }

private:
    static inline bool s_KeyStates[512] = { false };
    static inline bool s_LastKeyStates[512] = { false };
    static inline bool s_MouseStates[16] = { false };
    static inline bool s_LastMouseStates[16] = { false };

    static inline glm::vec2 s_MousePosition = { 0.0f, 0.0f };
    static inline glm::vec2 s_LastMousePosition = { 0.0f, 0.0f };
    static inline float s_MouseWheelDelta = 0.0f;
    static inline float s_CurrentMouseWheelDelta = 0.0f;
};

} // namespace CHEngine

#endif // CH_INPUT_H
