#ifndef CH_SCENE_EVENTS_H
#define CH_SCENE_EVENTS_H

#include "engine/core/events.h"
#include "entt/entt.hpp"
#include <string>

namespace CHEngine
{

// Project Events
class ProjectCreatedEvent : public Event
{
public:
    ProjectCreatedEvent(const std::string &name, const std::string &path)
        : m_Name(name), m_Path(path)
    {
    }

    const std::string &GetProjectName() const
    {
        return m_Name;
    }
    const std::string &GetPath() const
    {
        return m_Path;
    }

    EVENT_CLASS_TYPE(ProjectCreated)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    std::string m_Name;
    std::string m_Path;
};

class ProjectOpenedEvent : public Event
{
public:
    ProjectOpenedEvent(const std::string &path) : m_Path(path)
    {
    }

    const std::string &GetPath() const
    {
        return m_Path;
    }

    EVENT_CLASS_TYPE(ProjectOpened)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    std::string m_Path;
};

class SceneOpenedEvent : public Event
{
public:
    SceneOpenedEvent(const std::string &path) : m_Path(path)
    {
    }

    const std::string &GetPath() const
    {
        return m_Path;
    }

    EVENT_CLASS_TYPE(SceneOpened)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    std::string m_Path;
};

// Selection Events
class EntitySelectedEvent : public Event
{
public:
    EntitySelectedEvent(entt::entity entity, class Scene *scene, int meshIndex = -1)
        : m_Entity(entity), m_Scene(scene), m_MeshIndex(meshIndex)
    {
    }

    entt::entity GetEntity() const
    {
        return m_Entity;
    }
    class Scene *GetScene() const
    {
        return m_Scene;
    }
    int GetMeshIndex() const
    {
        return m_MeshIndex;
    }

    EVENT_CLASS_TYPE(EntitySelected)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)

private:
    entt::entity m_Entity;
    class Scene *m_Scene;
    int m_MeshIndex = -1;
};

class ScenePlayEvent : public Event
{
public:
    ScenePlayEvent() = default;
    EVENT_CLASS_TYPE(ScenePlay)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class SceneStopEvent : public Event
{
public:
    SceneStopEvent() = default;
    EVENT_CLASS_TYPE(SceneStop)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppLaunchRuntimeEvent : public Event
{
public:
    AppLaunchRuntimeEvent() = default;
    EVENT_CLASS_TYPE(AppLaunchRuntime)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

class AppResetLayoutEvent : public Event
{
public:
    AppResetLayoutEvent() = default;
    EVENT_CLASS_TYPE(AppResetLayout)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace CHEngine

#endif // CH_SCENE_EVENTS_H
