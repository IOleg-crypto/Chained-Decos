#ifndef CH_EVENTS_H
#define CH_EVENTS_H

#include <functional>
#include <string>

namespace CHEngine
{
enum class EventType
{
    None = 0,

    // --- Core System Events (classes defined below) ---
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,

    // --- Scene Events (classes in engine/scene/scene_events.h) ---
    ProjectCreated,
    ProjectOpened,
    SceneOpened,
    SceneSaved,
    ScenePlay,
    SceneStop,
    SceneChangeRequest,
    EntitySelected,

    // --- Editor Events (classes in editor/editor_events.h) ---
    AppLaunchRuntime,
    AppResetLayout,
    AppSaveLayout,

    // --- UI Events ---
    ButtonPressed
};

enum EventCategory
{
    None = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
    EventCategoryMouseButton = 1 << 4,
    EventCategoryButton = 1 << 5
};

#define EVENT_CLASS_TYPE(type)                                                                                         \
    static EventType GetStaticType()                                                                                   \
    {                                                                                                                  \
        return EventType::type;                                                                                        \
    }                                                                                                                  \
    virtual EventType GetEventType() const override                                                                    \
    {                                                                                                                  \
        return GetStaticType();                                                                                        \
    }                                                                                                                  \
    virtual const char* GetName() const override                                                                       \
    {                                                                                                                  \
        return #type;                                                                                                  \
    }

#define EVENT_CLASS_CATEGORY(category)                                                                                 \
    virtual int GetCategoryFlags() const override                                                                      \
    {                                                                                                                  \
        return category;                                                                                               \
    }

class Event
{
public:
    virtual ~Event() = default;

    bool Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
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
    EventDispatcher(Event& event)
        : m_Event(event)
    {
    }

    template <typename T, typename F> bool Dispatch(const F& func)
    {
        if (m_Event.GetEventType() == T::GetStaticType())
        {
            m_Event.Handled |= func(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }

private:
    Event& m_Event;
};

using EventCallbackFn = std::function<void(Event&)>;

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
    KeyEvent(int keycode)
        : m_KeyCode(keycode)
    {
    }
    int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keycode, bool isRepeat = false)
        : KeyEvent(keycode),
          m_IsRepeat(isRepeat)
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
    KeyReleasedEvent(int keycode)
        : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(int keycode)
        : KeyEvent(keycode)
    {
    }
    EVENT_CLASS_TYPE(KeyTyped)
};

// Mouse Events
class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y)
        : m_MouseX(x),
          m_MouseY(y)
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
    MouseScrolledEvent(float xOffset, float yOffset)
        : m_XOffset(xOffset),
          m_YOffset(yOffset)
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
    MouseButtonEvent(int button, Action action)
        : m_Button(button),
          m_Action(action)
    {
    }

    int m_Button;
    Action m_Action;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button)
        : MouseButtonEvent(button, Action::Pressed)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button)
        : MouseButtonEvent(button, Action::Released)
    {
    }
    EVENT_CLASS_TYPE(MouseButtonReleased)
};

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
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(WindowClose)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
};

} // namespace CHEngine

#endif // CH_EVENTS_H
