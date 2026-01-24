#ifndef CH_EVENTS_H
#define CH_EVENTS_H

#include <entt/entt.hpp>
#include <functional>
#include <string>

namespace CHEngine
{
enum class EventType
{
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    ProjectCreated,
    ProjectOpened,
    SceneOpened,
    SceneSaved,
    ScenePlay,
    SceneStop,
    AppLaunchRuntime,
    AppResetLayout,
    EntitySelected
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
    EventCategoryMouseButton = 1 << 4
};

#define EVENT_CLASS_TYPE(type)                                                                     \
    static EventType GetStaticType()                                                               \
    {                                                                                              \
        return EventType::type;                                                                    \
    }                                                                                              \
    virtual EventType GetEventType() const override                                                \
    {                                                                                              \
        return GetStaticType();                                                                    \
    }                                                                                              \
    virtual const char *GetName() const override                                                   \
    {                                                                                              \
        return #type;                                                                              \
    }

#define EVENT_CLASS_CATEGORY(category)                                                             \
    virtual int GetCategoryFlags() const override                                                  \
    {                                                                                              \
        return category;                                                                           \
    }

class Event
{
public:
    virtual ~Event() = default;

    bool Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char *GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const
    {
        return GetName();
    }

    bool IsInCategory(EventCategory category)
    {
        return GetCategoryFlags() & category;
    }
};

class EventDispatcher
{
public:
    EventDispatcher(Event &event) : m_Event(event)
    {
    }

    template <typename T, typename F> bool Dispatch(const F &func)
    {
        if (m_Event.GetEventType() == T::GetStaticType())
        {
            m_Event.Handled |= func(static_cast<T &>(m_Event));
            return true;
        }
        return false;
    }

private:
    Event &m_Event;
};

using EventCallbackFn = std::function<void(Event &)>;

// Keyboard Events
class KeyEvent : public Event
{
public:
    int GetKeyCode() const
    {
        return m_KeyCode;
    }
    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
protected:
    KeyEvent(int keycode) : m_KeyCode(keycode)
    {
    }
    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keycode, bool isRepeat = false) : KeyEvent(keycode), m_IsRepeat(isRepeat)
    {
    }
    bool IsRepeat() const
    {
        return m_IsRepeat;
    }
    EVENT_CLASS_TYPE(KeyPressed)
private:
    bool m_IsRepeat;
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keycode) : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(int keycode) : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyTyped)
};

// Mouse Events
class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y)
    {
    }
    float GetX() const
    {
        return m_MouseX;
    }
    float GetY() const
    {
        return m_MouseY;
    }
    EVENT_CLASS_TYPE(MouseMoved)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_MouseX, m_MouseY;
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset)
    {
    }
    float GetXOffset() const
    {
        return m_XOffset;
    }
    float GetYOffset() const
    {
        return m_YOffset;
    }
    EVENT_CLASS_TYPE(MouseScrolled)
    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
private:
    float m_XOffset, m_YOffset;
};

class MouseButtonEvent : public Event
{
public:
    enum class Action
    {
        None = 0,
        Pressed,
        Released
    };

    int GetMouseButton() const
    {
        return m_Button;
    }

    Action GetAction() const
    {
        return m_Action;
    }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

protected:
    MouseButtonEvent(int button, Action action) : m_Button(button), m_Action(action)
    {
    }

    int m_Button;
    Action m_Action;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button) : MouseButtonEvent(button, Action::Pressed)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button) : MouseButtonEvent(button, Action::Released)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonReleased)
};

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

#endif // CH_EVENTS_H
