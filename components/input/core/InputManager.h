#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "../interfaces/IInputManager.h"
#include "raylib.h"
#include <functional>
#include <map>


namespace ChainedDecos
{

class InputManager : public IInputManager
{
public:
    static InputManager &Get()
    {
        static InputManager instance;
        return instance;
    }

    InputManager();
    ~InputManager() override = default;

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

    Vector2 GetMousePosition() const override;
    Vector2 GetMouseDelta() const override;
    bool IsMouseButtonPressed(int button) const override;
    bool IsMouseButtonDown(int button) const override;
    bool IsMouseButtonReleased(int button) const override;
    float GetMouseWheelMove() const override;

    void DisableCursor() override;
    void EnableCursor() override;
    bool IsCursorDisabled() const override;

    void SetEventCallback(const EventCallbackFn &callback) override
    {
        m_EventCallback = callback;
    }

private:
    bool m_initialized = false;
    Vector2 m_lastMousePosition = {0, 0};
    EventCallbackFn m_EventCallback;

    std::map<int, std::function<void()>> m_pressedActions;
    std::map<int, std::function<void()>> m_heldActions;
    std::map<int, std::function<void()>> m_releasedActions;
};

} // namespace ChainedDecos

#endif // INPUTMANAGER_H


