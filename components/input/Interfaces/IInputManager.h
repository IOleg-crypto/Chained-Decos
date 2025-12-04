#ifndef IINPUTMANAGER_H
#define IINPUTMANAGER_H

#include <functional>

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
};

#endif // IINPUTMANAGER_H
