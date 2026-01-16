#include "input_binding.h"
#include "input.h"
#include <raylib.h>

namespace CHEngine
{

bool InputBinding::ModifiersMatch() const
{
    bool shiftPressed = Input::IsKeyDown(KEY_LEFT_SHIFT) || Input::IsKeyDown(KEY_RIGHT_SHIFT);
    bool ctrlPressed = Input::IsKeyDown(KEY_LEFT_CONTROL) || Input::IsKeyDown(KEY_RIGHT_CONTROL);
    bool altPressed = Input::IsKeyDown(KEY_LEFT_ALT) || Input::IsKeyDown(KEY_RIGHT_ALT);

    if (RequireShift && !shiftPressed)
        return false;
    if (RequireCtrl && !ctrlPressed)
        return false;
    if (RequireAlt && !altPressed)
        return false;

    // If we don't require a modifier but it's pressed, that's also a mismatch
    // (prevents Shift+W from triggering plain W binding)
    if (!RequireShift && shiftPressed)
        return false;
    if (!RequireCtrl && ctrlPressed)
        return false;
    if (!RequireAlt && altPressed)
        return false;

    return true;
}

} // namespace CHEngine
