#include "events/key_event.h"
#include <sstream>

namespace CHEngine
{

std::string KeyPressedEvent::ToString() const
{
    std::stringstream ss;
    ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
    return ss.str();
}

std::string KeyReleasedEvent::ToString() const
{
    std::stringstream ss;
    ss << "KeyReleasedEvent: " << m_KeyCode;
    return ss.str();
}

std::string KeyTypedEvent::ToString() const
{
    std::stringstream ss;
    ss << "KeyTypedEvent: " << m_KeyCode;
    return ss.str();
}

} // namespace CHEngine


