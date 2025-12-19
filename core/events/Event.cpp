#include "core/events/Event.h"

namespace ChainedDecos
{

std::ostream &operator<<(std::ostream &os, const Event &e)
{
    return os << e.ToString();
}

} // namespace ChainedDecos
