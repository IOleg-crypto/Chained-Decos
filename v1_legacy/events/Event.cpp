#include "events/event.h"

namespace CHEngine
{

std::ostream &operator<<(std::ostream &os, const Event &e)
{
    return os << e.ToString();
}

} // namespace CHEngine


