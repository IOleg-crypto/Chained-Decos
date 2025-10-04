#include "EventSystem.h"

// Global event system instance
EventSystem g_eventSystem;

void EventSystem::Clear()
{
    m_subscribers.clear();
}