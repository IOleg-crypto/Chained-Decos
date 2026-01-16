#ifndef CH_INPUT_CONTEXT_H
#define CH_INPUT_CONTEXT_H

#include "input_action.h"
#include "input_binding.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{

// Represents a collection of actions and bindings for a specific game mode
// Examples: "Gameplay", "Menu", "Driving", "Spectator"
class InputContext
{
public:
    InputContext(const std::string &name);

    const std::string &GetName() const
    {
        return m_Name;
    }

    // Action management
    void RegisterAction(const std::string &name, InputActionType type);
    InputAction *GetAction(const std::string &name);
    const std::unordered_map<std::string, std::shared_ptr<InputAction>> &GetActions() const
    {
        return m_Actions;
    }

    // Binding management
    void AddBinding(const InputBinding &binding);
    void RemoveBinding(const std::string &actionName, int keyCode);
    void ClearBindings();
    const std::vector<InputBinding> &GetBindings() const
    {
        return m_Bindings;
    }

    // Get all bindings for a specific action
    std::vector<InputBinding> GetBindingsForAction(const std::string &actionName) const;

    // Process input (called by InputManager)
    void ProcessKeyPressed(int keyCode);
    void ProcessKeyReleased(int keyCode);
    void ProcessAxisInput();

private:
    std::string m_Name;
    std::unordered_map<std::string, std::shared_ptr<InputAction>> m_Actions;
    std::vector<InputBinding> m_Bindings;

    // Axis accumulation (for 2D movement)
    std::unordered_map<std::string, Vector2> m_AxisAccumulators;
};

} // namespace CHEngine

#endif // CH_INPUT_CONTEXT_H
