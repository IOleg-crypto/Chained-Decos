#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "core/macros.h"
#include "core/object/Object.h"
#include <cstdint>
#include <functional>
#include <raylib.h>
#include <unordered_map>

// InputManager - Regular class with DI
// Enhanced input manager with support for different input types
class InputManager : public Object
{
    REGISTER_CLASS(InputManager, Object)
    DISABLE_COPY_AND_MOVE(InputManager)

public:
    enum class InputType : uint8_t
    {
        PRESSED,
        HELD,
        RELEASED
    };

    // Constructor and Destructor
    InputManager();
    ~InputManager();

    // Lifecycle
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Action registration
    void RegisterAction(int key, const std::function<void()> &action,
                        InputType type = InputType::PRESSED);
    void RegisterPressedAction(int key, const std::function<void()> &action);
    void RegisterHeldAction(int key, const std::function<void()> &action);
    void RegisterReleasedAction(int key, const std::function<void()> &action);

    void UnregisterAction(int key, InputType type);
    void ClearActions();
    void ProcessInput() const;

    // Direct key queries
    bool IsKeyPressed(int key) const;
    bool IsKeyDown(int key) const;
    bool IsKeyReleased(int key) const;

    // Mouse
    Vector2 GetMousePosition() const;
    Vector2 GetMouseDelta() const;
    bool IsMouseButtonPressed(int button) const;
    bool IsMouseButtonDown(int button) const;
    bool IsMouseButtonReleased(int button) const;

    // Cursor control
    void DisableCursor();
    void EnableCursor();
    bool IsCursorDisabled() const;

private:
    std::unordered_map<int, std::function<void()>> m_pressedActions;
    std::unordered_map<int, std::function<void()>> m_heldActions;
    std::unordered_map<int, std::function<void()>> m_releasedActions;

    Vector2 m_lastMousePosition;
    bool m_initialized;
};

#endif // INPUTMANAGER_H
