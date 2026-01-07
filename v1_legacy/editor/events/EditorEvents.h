#ifndef CD_EDITOR_EVENTS_EDITOREVENTS_H
#define CD_EDITOR_EVENTS_EDITOREVENTS_H

#include "events/event.h"
#include <string>

namespace CHEngine
{

// Event fired when an object is selected in the editor
class ObjectSelectedEvent : public Event
{
public:
    explicit ObjectSelectedEvent(int index);
    int GetObjectIndex() const;

    EVENT_CLASS_TYPE(ObjectSelected)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    int m_objectIndex;
};

// Event fired when an object is modified
class ObjectModifiedEvent : public Event
{
public:
    explicit ObjectModifiedEvent(int index);
    int GetObjectIndex() const;

    EVENT_CLASS_TYPE(ObjectModified)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    int m_objectIndex;
};

// Event fired when a scene is loaded
class SceneLoadedEvent : public Event
{
public:
    explicit SceneLoadedEvent(const std::string &path);
    const std::string &GetPath() const;

    EVENT_CLASS_TYPE(SceneLoaded)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    std::string m_path;
};

// Event fired when scene is modified
class SceneModifiedEvent : public Event
{
public:
    SceneModifiedEvent();

    EVENT_CLASS_TYPE(SceneModified)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace CHEngine

#endif // CD_EDITOR_EVENTS_EDITOREVENTS_H
