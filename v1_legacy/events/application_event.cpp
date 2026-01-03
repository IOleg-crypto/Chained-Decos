#include "events/application_event.h"
#include <sstream>

namespace CHEngine
{

std::string WindowResizeEvent::ToString() const
{
    std::stringstream ss;
    ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
    return ss.str();
}

unsigned int WindowResizeEvent::GetWidth() const
{
    return m_Width;
}
WindowResizeEvent::WindowResizeEvent(unsigned int width, unsigned int height)
    : m_Width(width), m_Height(height)
{
}
unsigned int WindowResizeEvent::GetHeight() const
{
    return m_Height;
}
} // namespace CHEngine


