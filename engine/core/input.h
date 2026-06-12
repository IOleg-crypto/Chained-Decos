#ifndef CH_INPUT_H
#define CH_INPUT_H

#include "engine/foundation/base.h"
#include "engine/foundation/timestep.h"
#include "key_codes.h"
#include "mouse_codes.h"
#include <glm/vec2.hpp>

namespace Chained::Core::Input
{
    void Init();
    void Shutdown();
    void Update(Timestep ts);

    bool IsKeyPressed(KeyCode key);
    bool IsKeyDown(KeyCode key);
    bool IsKeyReleased(KeyCode key);
    bool IsKeyUp(KeyCode key);

    bool IsMouseButtonPressed(MouseCode button);
    bool IsMouseButtonDown(MouseCode button);
    bool IsMouseButtonReleased(MouseCode button);
    bool IsMouseButtonUp(MouseCode button);

    glm::vec2 GetMousePosition();
    glm::vec2 GetMouseDelta();
    float GetMouseWheelMove();

    void OnKey(int key, bool pressed);
    void OnMouseButton(int button, bool pressed);
    void OnMouseMove(float x, float y);
    void OnMouseScroll(float xOffset, float yOffset);
}

#endif // CH_INPUT_H