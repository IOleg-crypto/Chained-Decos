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

#include "servers/input/Interfaces/IInputManager.h"
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
CHAINEDDECOSENGINE_API class InputManager : public IInputManager
{
public:
    InputManager() = default;
    ~InputManager() override = default;

    InputManager(const InputManager &other) = delete;
    InputManager(InputManager &&other) = delete;

    // IInputManager interface
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    void RegisterAction(int key, const std::function<void()> &action,
                        InputType type = InputType::PRESSED) override;
    void RegisterPressedAction(int key, const std::function<void()> &action) override;
    void RegisterHeldAction(int key, const std::function<void()> &action) override;
    void RegisterReleasedAction(int key, const std::function<void()> &action) override;

    void UnregisterAction(int key, InputType type) override;
    void ClearActions() override;

    void ProcessInput() const override;

    bool IsKeyPressed(int key) const override;
    bool IsKeyDown(int key) const override;
    bool IsKeyReleased(int key) const override;

private:
    std::unordered_map<int, std::function<void()>> m_pressedActions;
    std::unordered_map<int, std::function<void()>> m_heldActions;
    std::unordered_map<int, std::function<void()>> m_releasedActions;
};

#endif // INPUTMANAGER_H
