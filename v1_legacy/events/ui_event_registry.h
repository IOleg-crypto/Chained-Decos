#ifndef CD_EVENTS_UI_EVENT_REGISTRY_H
#define CD_EVENTS_UI_EVENT_REGISTRY_H

#include "core/log.h"
#include <functional>
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class UIEventRegistry
{
public:
    using EventCallback = std::function<void()>;

    static void Register(const std::string &eventId, EventCallback callback)
    {
        s_Events[eventId] = callback;
    }

    static void Trigger(const std::string &eventId)
    {
        auto it = s_Events.find(eventId);
        if (it != s_Events.end())
        {
            if (it->second)
                it->second();
            CD_CORE_INFO("[UIEventRegistry] Triggered event: %s", eventId.c_str());
        }
    }

    static void Clear()
    {
        s_Events.clear();
    }

private:
    inline static std::unordered_map<std::string, EventCallback> s_Events;
};
} // namespace CHEngine

#endif // CD_EVENTS_UI_EVENT_REGISTRY_H
