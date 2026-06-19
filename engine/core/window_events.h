#ifndef CH_WINDOW_EVENTS_H
#define CH_WINDOW_EVENTS_H

#include "engine/core/events.h"

namespace Chained
{

class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
        : m_Width(width),
          m_Height(height)
    {
    }
    unsigned int GetWidth() const
    {
        return m_Width;
    }
    unsigned int GetHeight() const
    {
        return m_Height;
    }
    std::string ToString() const override
    {
        return "WindowResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Height);
    }
    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    unsigned int m_Width, m_Height;
};

class WindowCloseEvent : public Event
{
public:
    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace Chained

#endif