#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include "core/object/event/Core/Event.h"
#include <functional>
#include <map>
#include <memory>
#include <typeindex>
#include <vector>

// Type alias for event handler functions
using EventHandler = std::function<void(const Event &)>;

// EventDispatcher - Central hub for event-based communication
// Allows modules to communicate without direct dependencies
class EventDispatcher
{
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    // Prevent copying
    EventDispatcher(const EventDispatcher &) = delete;
    EventDispatcher &operator=(const EventDispatcher &) = delete;

    // Subscribe to events of a specific type
    // Returns subscription ID that can be used to unsubscribe
    template <typename T> int Subscribe(std::function<void(const T &)> handler)
    {
        static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");

        const std::type_index typeId = std::type_index(typeid(T));

        // Wrap the typed handler in a generic handler
        EventHandler genericHandler = [handler](const Event &event)
        { handler(static_cast<const T &>(event)); };

        int subscriptionId = m_nextSubscriptionId++;
        m_handlers[typeId].push_back({subscriptionId, genericHandler});

        return subscriptionId;
    }

    // Unsubscribe from events using subscription ID
    void Unsubscribe(int subscriptionId);

    // Publish an event to all subscribers
    template <typename T> void Publish(const T &event)
    {
        static_assert(std::is_base_of_v<Event, T>, "T must derive from Event");

        const std::type_index typeId = std::type_index(typeid(T));

        auto it = m_handlers.find(typeId);
        if (it != m_handlers.end())
        {
            // Call all handlers for this event type
            for (const auto &[id, handler] : it->second)
            {
                handler(event);
            }
        }
    }

    // Clear all event handlers
    void ClearAllHandlers();

    // Get number of subscribers for a specific event type
    template <typename T> size_t GetSubscriberCount() const
    {
        const std::type_index typeId = std::type_index(typeid(T));
        auto it = m_handlers.find(typeId);
        return it != m_handlers.end() ? it->second.size() : 0;
    }

private:
    struct HandlerEntry
    {
        int subscriptionId;
        EventHandler handler;
    };

    std::map<std::type_index, std::vector<HandlerEntry>> m_handlers;
    int m_nextSubscriptionId = 1;
};

#endif // EVENT_DISPATCHER_H
