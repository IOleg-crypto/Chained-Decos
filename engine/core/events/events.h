#ifndef CH_EVENTS_H
#define CH_EVENTS_H

#include <functional>
#include <string>
#include <deque>
#include <memory>

namespace Chained
{
    enum class EventType
    {
        None = 0,
        WindowClose, WindowResize,
        KeyPressed, KeyReleased, KeyTyped,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
        ProjectCreated, ProjectOpened, SceneOpened, SceneSaved,
        ScenePlay, SceneStop, SceneChangeRequest, EntitySelected,
        AppLaunchRuntime, AppResetLayout, AppSaveLayout, ViewportFocusEntity,
        ButtonPressed, Undo, Redo
    };

    enum EventCategory
    {
        NoneCategory = 0,
        EventCategoryApplication = 1 << 0,
        EventCategoryInput       = 1 << 1,
        EventCategoryKeyboard    = 1 << 2,
        EventCategoryMouse       = 1 << 3,
        EventCategoryMouseButton = 1 << 4,
        EventCategoryButton      = 1 << 5,
        EventCategoryEditor      = 1 << 6
    };

#define EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() { return EventType::type; } \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
    virtual int GetCategoryFlags() const override { return category; }

    class Event
    {
    public:
        virtual ~Event() = default;
        bool Handled = false;

        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string ToString() const { return GetName(); }
        bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
    };

    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event) : m_Event(event) {}
        
        template <typename T, typename F> 
        bool Dispatch(const F& func)
        {
            if (m_Event.GetEventType() == T::GetStaticType()) {
                m_Event.Handled |= func(static_cast<T&>(m_Event));
                return true;
            }
            return false;
        }
    private:
        Event& m_Event;
    };

    class EventQueue
    {
    public:
        void Push(std::unique_ptr<Event> event)
        {
            m_Queue.push_back(std::move(event));
        }

        template<typename T, typename... Args>
        void Enqueue(Args&&... args)
        {
            m_Queue.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        }

        void Process(const std::function<void(Event&)>& callback)
        {
            while (!m_Queue.empty())
            {
                auto event = std::move(m_Queue.front());
                m_Queue.pop_front();
                callback(*event);
            }
        }

        bool IsEmpty() const { return m_Queue.empty(); }

    private:
        std::deque<std::unique_ptr<Event>> m_Queue;
    };

    using EventCallbackFn = std::function<void(Event&)>;
} // namespace Chained

#endif // CH_EVENTS_H