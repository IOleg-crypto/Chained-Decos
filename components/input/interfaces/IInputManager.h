#ifndef IINPUTMANAGER_H
#define IINPUTMANAGER_H

#include <cstdint>
#include <functional>
#include <string>


namespace ChainedDecos
{
class Event;
}

// Abstract interface for InputManager
class IInputManager
{
public:
    enum class InputType : uint8_t
    {
        PRESSED, // Single press
        HELD,    // Continuous while held
        RELEASED // On key release
    };

    virtual ~IInputManager() = default;

    // Initialization
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime) = 0;

    // Register different types of input actions
    virtual void RegisterAction(int key, const std::function<void()> &action,
                                InputType type = InputType::PRESSED) = 0;
    virtual void RegisterPressedAction(int key, const std::function<void()> &action) = 0;
    virtual void RegisterHeldAction(int key, const std::function<void()> &action) = 0;
    virtual void RegisterReleasedAction(int key, const std::function<void()> &action) = 0;

    // Remove actions
    virtual void UnregisterAction(int key, InputType type) = 0;
    virtual void ClearActions() = 0;

    // Process all registered input actions
    virtual void ProcessInput() const = 0;

    // Direct input queries
    virtual bool IsKeyPressed(int key) const = 0;
    virtual bool IsKeyDown(int key) const = 0;
    virtual bool IsKeyReleased(int key) const = 0;

    // Mouse input queries
    virtual struct Vector2 GetMousePosition() const = 0;
    virtual struct Vector2 GetMouseDelta() const = 0;
    virtual bool IsMouseButtonPressed(int button) const = 0;
    virtual bool IsMouseButtonDown(int button) const = 0;
    virtual bool IsMouseButtonReleased(int button) const = 0;
    virtual float GetMouseWheelMove() const = 0;

    // Cursor control
    virtual void DisableCursor() = 0;
    virtual void EnableCursor() = 0;
    virtual bool IsCursorDisabled() const = 0;

    // Event system integration
    using EventCallbackFn = std::function<void(ChainedDecos::Event &)>;
    virtual void SetEventCallback(const EventCallbackFn &callback) = 0;
};

#endif // IINPUTMANAGER_H
