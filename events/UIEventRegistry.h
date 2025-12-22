#ifndef UI_EVENT_REGISTRY_H
#define UI_EVENT_REGISTRY_H

#include <functional>
#include <raylib.h>
#include <string>
#include <unordered_map>


namespace ChainedDecos
{
class UIEventRegistry
{
public:
    using EventCallback = std::function<void()>;

    static UIEventRegistry &Get()
    {
        static UIEventRegistry instance;
        return instance;
    }

    void Register(const std::string &eventId, EventCallback callback)
    {
        m_events[eventId] = callback;
    }

    void Trigger(const std::string &eventId)
    {
        auto it = m_events.find(eventId);
        if (it != m_events.end())
        {
            if (it->second)
                it->second();
            TraceLog(LOG_INFO, "[UIEventRegistry] Triggered event: %s", eventId.c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "[UIEventRegistry] Event not found: %s", eventId.c_str());
        }
    }

    void Clear()
    {
        m_events.clear();
    }

private:
    UIEventRegistry() = default;
    std::unordered_map<std::string, EventCallback> m_events;
};
} // namespace ChainedDecos

#endif // UI_EVENT_REGISTRY_H
