#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <functional>
#include <raylib.h>
#include <unordered_map>

// InputManager - Static Singleton
// Enhanced input manager with support for different input types
class InputManager
{
public:
    enum class InputType
    {
        PRESSED, // Single press
        HELD,    // Continuous hold
        RELEASED // Release
    };

    // Static Singleton - як у Godot!
    static InputManager &Get()
    {
        static InputManager instance;
        return instance;
    }

    // Заборонити копіювання
    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

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
    // Private constructor для Singleton
    InputManager() = default;
    ~InputManager() = default;

    std::unordered_map<int, std::function<void()>> m_pressedActions;
    std::unordered_map<int, std::function<void()>> m_heldActions;
    std::unordered_map<int, std::function<void()>> m_releasedActions;

    Vector2 m_lastMousePosition = {0, 0};
    bool m_initialized = false;
};

#endif // INPUTMANAGER_H
