#ifndef APPLICATION_EVENT_H
#define APPLICATION_EVENT_H

#include "events/Event.h"

namespace ChainedDecos
{

class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height);

    unsigned int GetWidth() const;
    unsigned int GetHeight() const;

    std::string ToString() const override;

    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    unsigned int m_Width, m_Height;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppTickEvent : public Event
{
public:
    AppTickEvent() = default;

    EVENT_CLASS_TYPE(AppTick)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppUpdateEvent : public Event
{
public:
    AppUpdateEvent() = default;

    EVENT_CLASS_TYPE(AppUpdate)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppRenderEvent : public Event
{
public:
    AppRenderEvent() = default;

    EVENT_CLASS_TYPE(AppRender)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
} // namespace ChainedDecos

#endif // APPLICATION_EVENT_H


