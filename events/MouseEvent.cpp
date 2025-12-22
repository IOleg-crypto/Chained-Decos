#include "events/MouseEvent.h"
#include <sstream>

namespace ChainedDecos
{

std::string MouseMovedEvent::ToString() const
{
    std::stringstream ss;
    ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
    return ss.str();
}

std::string MouseScrolledEvent::ToString() const
{
    std::stringstream ss;
    ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
    return ss.str();
}

std::string MouseButtonPressedEvent::ToString() const
{
    std::stringstream ss;
    ss << "MouseButtonPressedEvent: " << m_Button;
    return ss.str();
}

std::string MouseButtonReleasedEvent::ToString() const
{
    std::stringstream ss;
    ss << "MouseButtonReleasedEvent: " << m_Button;
    return ss.str();
}

} // namespace ChainedDecos


