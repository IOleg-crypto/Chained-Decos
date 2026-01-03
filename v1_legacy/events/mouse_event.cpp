#include "events/mouse_event.h"
#include <sstream>

namespace CHEngine
{

std::string MouseMovedEvent::ToString() const
{
    std::stringstream ss;
    ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
    return ss.str();
}

MouseMovedEvent::MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y){}

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

} // namespace CHEngine


