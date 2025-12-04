#ifndef SERVERS_INPUT_MANAGER_H
#define SERVERS_INPUT_MANAGER_H

#include <raylib.h>

namespace Servers
{

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    // Non-copyable
    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    void Update();

    // Key states
    static bool IsPressed(int key);
    static bool IsDown(int key);
    static bool IsReleased(int key);

    // Mouse states
    static bool IsMousePressed(int button);
    static bool IsMouseDown(int button);
    static bool IsMouseReleased(int button);
    static Vector2 GetMousePosition();
    static Vector2 GetMouseDelta();
    static float GetMouseWheel();
};

} // namespace Servers

#endif // SERVERS_INPUT_MANAGER_H
