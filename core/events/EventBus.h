#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

// Event Bus - центральна система подій
// Дозволяє сервісам спілкуватися без прямих залежностей
class EventBus
{
public:
    // Підписка на подію
    template <typename EventType> int Subscribe(std::function<void(const EventType &)> handler)
    {
        const std::type_index typeId = std::type_index(typeid(EventType));

        // Створюємо обгортку для type erasure
        auto wrapper = [handler](const void *eventPtr)
        {
            const EventType *event = static_cast<const EventType *>(eventPtr);
            handler(*event);
        };

        int subscriptionId = m_nextSubscriptionId++;
        m_handlers[typeId].push_back({subscriptionId, wrapper});

        return subscriptionId;
    }

    // Відписка від події
    void Unsubscribe(int subscriptionId)
    {
        for (auto &[typeId, handlers] : m_handlers)
        {
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                                          [subscriptionId](const auto &pair)
                                          { return pair.first == subscriptionId; }),
                           handlers.end());
        }
    }

    // Публікація події
    template <typename EventType> void Publish(const EventType &event)
    {
        const std::type_index typeId = std::type_index(typeid(EventType));

        auto it = m_handlers.find(typeId);
        if (it != m_handlers.end())
        {
            for (const auto &[id, handler] : it->second)
            {
                handler(&event);
            }
        }
    }

    // Очистити всі підписки
    void Clear()
    {
        m_handlers.clear();
    }

    // Singleton доступ
    static EventBus &Instance()
    {
        static EventBus instance;
        return instance;
    }

private:
    using Handler = std::function<void(const void *)>;
    std::unordered_map<std::type_index, std::vector<std::pair<int, Handler>>> m_handlers;
    int m_nextSubscriptionId = 0;
};

#endif // EVENT_BUS_H
