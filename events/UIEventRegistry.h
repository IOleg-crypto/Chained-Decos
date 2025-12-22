#ifndef UI_EVENT_REGISTRY_H
#define UI_EVENT_REGISTRY_H

#include "core/Log.h"
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
            CD_CORE_INFO("[UIEventRegistry] Triggered event: %s", eventId.c_str());
        }
        else
        {
            CD_CORE_WARN("[UIEventRegistry] Event not found: %s", eventId.c_str());
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
