#include "editor/events/EditorEvents.h"

namespace CHEngine
{

ObjectSelectedEvent::ObjectSelectedEvent(int index) : m_objectIndex(index)
{
}
int ObjectSelectedEvent::GetObjectIndex() const
{
    return m_objectIndex;
}

ObjectModifiedEvent::ObjectModifiedEvent(int index) : m_objectIndex(index)
{
}
int ObjectModifiedEvent::GetObjectIndex() const
{
    return m_objectIndex;
}

SceneLoadedEvent::SceneLoadedEvent(const std::string &path) : m_path(path)
{
}
const std::string &SceneLoadedEvent::GetPath() const
{
    return m_path;
}

SceneModifiedEvent::SceneModifiedEvent() = default;

} // namespace CHEngine
