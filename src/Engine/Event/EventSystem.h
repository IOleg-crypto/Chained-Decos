#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <any>
#include <typeindex>
#include <memory>
#include <algorithm>

class EventSystem
{
public:
    EventSystem() = default;
    ~EventSystem() = default;

    // Subscribe to an event with a specific type
    template<typename EventType>
    void Subscribe(const std::string& eventName, std::function<void(const EventType&)> callback)
    {
        [[maybe_unused]] auto typeIndex = std::type_index(typeid(EventType));

        // Create the event type entry if it doesn't exist
        if (m_subscribers.find(eventName) == m_subscribers.end())
        {
            m_subscribers[eventName] = std::make_shared<EventSubscribers>();
        }

        // Add the callback to the appropriate type list
        auto& typeCallbacks = m_subscribers[eventName]->GetCallbacks<EventType>();
        typeCallbacks.push_back(std::make_shared<EventCallback<EventType>>(callback));
    }

    // Unsubscribe from an event
    template<typename EventType>
    void Unsubscribe(const std::string& eventName, std::function<void(const EventType&)> callback)
    {
        auto it = m_subscribers.find(eventName);
        if (it != m_subscribers.end())
        {
            auto& typeCallbacks = it->second->GetCallbacks<EventType>();
            // Remove matching callbacks (simple linear search for now)
            typeCallbacks.erase(
                std::remove_if(typeCallbacks.begin(), typeCallbacks.end(),
                    [&callback](const std::shared_ptr<EventCallbackBase>& cb) {
                        auto typedCb = std::dynamic_pointer_cast<EventCallback<EventType>>(cb);
                        if (typedCb) {
                            return typedCb->callback.target_type() == callback.target_type();
                        }
                        return false;
                    }),
                typeCallbacks.end()
            );
        }
    }

    // Emit an event
    template<typename EventType>
    void Emit(const std::string& eventName, const EventType& eventData)
    {
        auto it = m_subscribers.find(eventName);
        if (it != m_subscribers.end())
        {
            auto& typeCallbacks = it->second->GetCallbacks<EventType>();
            for (auto& callback : typeCallbacks)
            {
                if (auto typedCallback = std::dynamic_pointer_cast<EventCallback<EventType>>(callback))
                {
                    typedCallback->callback(eventData);
                }
            }
        }
    }

    // Clear all subscribers
    void Clear();

private:
    // Base class for event callbacks
    struct EventCallbackBase
    {
        virtual ~EventCallbackBase() = default;
    };

    // Typed callback wrapper
    template<typename EventType>
    struct EventCallback : public EventCallbackBase
    {
        std::function<void(const EventType&)> callback;
        explicit EventCallback(std::function<void(const EventType&)> cb) : callback(std::move(cb)) {}
    };

    // Container for all callbacks of different types for an event
    struct EventSubscribers
    {
        std::unordered_map<std::type_index, std::shared_ptr<std::vector<std::shared_ptr<EventCallbackBase>>>> m_typeCallbacks;

        template<typename EventType>
        std::vector<std::shared_ptr<EventCallbackBase>>& GetCallbacks()
        {
            auto typeIndex = std::type_index(typeid(EventType));
            if (m_typeCallbacks.find(typeIndex) == m_typeCallbacks.end())
            {
                m_typeCallbacks[typeIndex] = std::make_shared<std::vector<std::shared_ptr<EventCallbackBase>>>();
            }
            return *m_typeCallbacks[typeIndex];
        }
    };

    std::unordered_map<std::string, std::shared_ptr<EventSubscribers>> m_subscribers;
};

// Global event system instance
extern EventSystem g_eventSystem;

#endif // EVENTSYSTEM_H