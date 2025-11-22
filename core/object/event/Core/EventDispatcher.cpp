#include "core/object/event/Core/EventDispatcher.h"

void EventDispatcher::Unsubscribe(int subscriptionId)
{
    // Search through all event types to find and remove the subscription
    for (auto &[typeId, handlers] : m_handlers)
    {
        auto it = std::remove_if(handlers.begin(), handlers.end(),
                                 [subscriptionId](const HandlerEntry &entry)
                                 { return entry.subscriptionId == subscriptionId; });

        if (it != handlers.end())
        {
            handlers.erase(it, handlers.end());
            return;
        }
    }
}

void EventDispatcher::ClearAllHandlers()
{
    m_handlers.clear();
}
