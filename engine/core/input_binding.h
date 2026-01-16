#ifndef CH_INPUT_BINDING_H
#define CH_INPUT_BINDING_H

#include <string>

namespace CHEngine
{

enum class InputAxis
{
    None,
    X,
    Y
};

// Represents a binding of a physical key to an action
struct InputBinding
{
    std::string ActionName; // Which action this binding triggers
    int KeyCode;            // Physical key (KEY_W, KEY_SPACE, etc.)

    // For axis inputs
    InputAxis Axis = InputAxis::None;
    float Scale = 1.0f; // Multiplier (use -1.0f for inversion)

    // Modifiers
    bool RequireShift = false;
    bool RequireCtrl = false;
    bool RequireAlt = false;

    // Check if modifiers match current state
    bool ModifiersMatch() const;
};

} // namespace CHEngine

#endif // CH_INPUT_BINDING_H
