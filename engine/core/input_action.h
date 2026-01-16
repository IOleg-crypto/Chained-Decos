#ifndef CH_INPUT_ACTION_H
#define CH_INPUT_ACTION_H

#include "engine/core/math_types.h"
#include <functional>
#include <string>
#include <vector>


namespace CHEngine
{

enum class InputActionType
{
    Button, // Simple press/release (Jump, Shoot, Interact)
    Axis1D, // Single axis value (Throttle, Zoom)
    Axis2D  // Two-dimensional input (Movement, Camera Look)
};

// Represents an abstract input action (e.g., "Jump", "Move", "Interact")
class InputAction
{
public:
    InputAction(const std::string &name, InputActionType type);

    const std::string &GetName() const
    {
        return m_Name;
    }
    InputActionType GetType() const
    {
        return m_Type;
    }

    // Button callbacks
    void OnPressed();
    void OnReleased();

    // Axis callbacks
    void OnAxis1D(float value);
    void OnAxis2D(Vector2 value);

    // Subscribe to action events
    void SubscribePressed(std::function<void()> callback);
    void SubscribeReleased(std::function<void()> callback);
    void SubscribeAxis1D(std::function<void(float)> callback);
    void SubscribeAxis2D(std::function<void(Vector2)> callback);

    // Clear all subscribers
    void ClearSubscribers();

private:
    std::string m_Name;
    InputActionType m_Type;

    // Callback lists
    std::vector<std::function<void()>> m_PressedCallbacks;
    std::vector<std::function<void()>> m_ReleasedCallbacks;
    std::vector<std::function<void(float)>> m_Axis1DCallbacks;
    std::vector<std::function<void(Vector2)>> m_Axis2DCallbacks;
};

} // namespace CHEngine

#endif // CH_INPUT_ACTION_H
