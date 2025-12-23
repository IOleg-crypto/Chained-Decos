#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "../interfaces/IInputManager.h"
#include "raylib.h"
#include <functional>
#include <map>

namespace CHEngine
{

class InputManager : public IInputManager
{
public:
    InputManager();
    ~InputManager() override = default;

    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    void RegisterAction(int key, const std::function<void()> &action,
                        IInputManager::InputType type = IInputManager::InputType::PRESSED) override;
    void RegisterPressedAction(int key, const std::function<void()> &action) override;
    void RegisterHeldAction(int key, const std::function<void()> &action) override;
    void RegisterReleasedAction(int key, const std::function<void()> &action) override;

    void UnregisterAction(int key, IInputManager::InputType type) override;
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

} // namespace CHEngine

#endif // INPUTMANAGER_H
