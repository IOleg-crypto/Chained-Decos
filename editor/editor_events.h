#ifndef CH_EDITOR_EVENTS_H
#define CH_EDITOR_EVENTS_H

#include "engine/core/events.h"
#include "engine/scene/entity.h"

namespace Chained
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

// Event to signal focusing on a specific entity in the viewport
class ViewportFocusEntityEvent : public Event
{
public:
    ViewportFocusEntityEvent(Entity entity)
        : m_Entity(entity)
    {
    }
    Entity GetEntity() const
    {
        return m_Entity;
    }

    EVENT_CLASS_TYPE(ViewportFocusEntity)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    Entity m_Entity;
};

// Undo system events
class UndoEvent : public Event
{
public:
    UndoEvent() = default;
    EVENT_CLASS_TYPE(Undo)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class RedoEvent : public Event
{
public:
    RedoEvent() = default;
    EVENT_CLASS_TYPE(Redo)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace Chained

#endif // CH_EDITOR_EVENTS_H
