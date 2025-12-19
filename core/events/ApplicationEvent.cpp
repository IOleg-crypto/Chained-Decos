#include "core/events/ApplicationEvent.h"
#include <sstream>

namespace ChainedDecos
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
} // namespace ChainedDecos
