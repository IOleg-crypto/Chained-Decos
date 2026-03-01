#ifndef CH_EDITOR_EVENTS_H
#define CH_EDITOR_EVENTS_H

#include "engine/core/events.h"

namespace CHEngine
{
// Event to trigger an layout reset.
class AppResetLayoutEvent : public Event
{
public:
    AppResetLayoutEvent() = default;
    EVENT_CLASS_TYPE(AppResetLayout)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

// Event to save current window layout.
class AppSaveLayoutEvent : public Event
{
public:
    AppSaveLayoutEvent() = default;
    EVENT_CLASS_TYPE(AppSaveLayout)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

// Event to trigger launching the game in runtime mode.
class AppLaunchRuntimeEvent : public Event
{
public:
    AppLaunchRuntimeEvent() = default;
    EVENT_CLASS_TYPE(AppLaunchRuntime)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};
} // namespace CHEngine

#endif // CH_EDITOR_EVENTS_H
