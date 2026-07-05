#ifndef CH_MOUSE_CODES_H
#define CH_MOUSE_CODES_H

#include <cstdint>

namespace Chained
{
enum class MouseCode : int32_t
{
    // Standard Mouse Buttons
    ButtonLeft = 0,
    ButtonRight = 1,
    ButtonMiddle = 2,
    ButtonSide = 3,
    ButtonExtra = 4,
    ButtonForward = 5,
    ButtonBack = 6,
};
}

#endif // CH_MOUSE_CODES_H
