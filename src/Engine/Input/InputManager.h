//
// Created by I#Oleg.
//

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#ifdef _WIN32
#ifdef CHAINEDDECOSENGINE_EXPORTS
#define CHAINEDDECOSENGINE_API __declspec(dllexport)
#else
#define CHAINEDDECOSENGINE_API __declspec(dllimport)
#endif
#else
#define CHAINEDDECOSENGINE_API
#endif

#include <functional>
#include <raylib.h>
#include <unordered_map>

//
// InputManager
// Enhanced input manager with support for different input types:
// - Single press actions (KEY_PRESSED)
// - Continuous hold actions (KEY_DOWN)
// - Release actions (KEY_RELEASED)
//
CHAINEDDECOSENGINE_API class InputManager
{
public:
    enum class InputType
    {
        PRESSED, // Single press
        HELD,    // Continuous while held
        RELEASED // On key release
    };

    InputManager() = default;
    ~InputManager() = default;

    InputManager(const InputManager &other) = delete;
    InputManager(InputManager &&other) = delete;

    // Register different types of input actions
    void RegisterAction(int key, const std::function<void()> &action,
                        InputType type = InputType::PRESSED);
    void RegisterPressedAction(int key, const std::function<void()> &action);
    void RegisterHeldAction(int key, const std::function<void()> &action);
    void RegisterReleasedAction(int key, const std::function<void()> &action);

    // Remove actions
    void UnregisterAction(int key, InputType type);
    void ClearActions();

    // Process all registered input actions
    void ProcessInput() const;

    // Direct input queries
    bool IsKeyPressed(int key) const { return ::IsKeyPressed(key); }
    bool IsKeyDown(int key) const { return ::IsKeyDown(key); }
    bool IsKeyReleased(int key) const { return ::IsKeyReleased(key); }


private:
    std::unordered_map<int, std::function<void()>> m_pressedActions;
    std::unordered_map<int, std::function<void()>> m_heldActions;
    std::unordered_map<int, std::function<void()>> m_releasedActions;
};

#endif // INPUTMANAGER_H
