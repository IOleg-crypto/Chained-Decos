#ifndef MODULE_CONTEXT_H
#define MODULE_CONTEXT_H

#include "core/object/event/Core/EventDispatcher.h"
#include "core/object/kernel/Core/Kernel.h"
#include <memory>
#include <stdexcept>
#include <typeindex>


// ModuleContext - Provides controlled access to engine services for modules
// This decouples modules from direct Kernel dependency and provides event system access
class ModuleContext
{
public:
    explicit ModuleContext(Kernel *kernel, EventDispatcher *eventDispatcher)
        : m_kernel(kernel), m_eventDispatcher(eventDispatcher)
    {
    }

    // Get a service from the kernel (type-safe)
    template <typename T> std::shared_ptr<T> GetService()
    {
        if (!m_kernel)
        {
            return nullptr;
        }
        return m_kernel->GetService<T>();
    }

    // Get a required service (throws if not found)
    template <typename T> std::shared_ptr<T> RequireService()
    {
        if (!m_kernel)
        {
            throw std::runtime_error("ModuleContext: Kernel is null");
        }
        return m_kernel->RequireService<T>();
    }

    // Check if a service exists
    template <typename T> bool HasService() const
    {
        if (!m_kernel)
        {
            return false;
        }
        return m_kernel->HasService<T>();
    }

    // Event system access
    EventDispatcher *GetEventDispatcher() const
    {
        return m_eventDispatcher;
    }

    // Subscribe to an event
    template <typename T> int Subscribe(std::function<void(const T &)> handler)
    {
        if (!m_eventDispatcher)
        {
            return -1;
        }
        return m_eventDispatcher->Subscribe<T>(handler);
    }

    // Publish an event
    template <typename T> void Publish(const T &event)
    {
        if (m_eventDispatcher)
        {
            m_eventDispatcher->Publish<T>(event);
        }
    }

    // Unsubscribe from an event
    void Unsubscribe(int subscriptionId)
    {
        if (m_eventDispatcher)
        {
            m_eventDispatcher->Unsubscribe(subscriptionId);
        }
    }

    // Direct kernel access (use sparingly, prefer GetService)
    Kernel *GetKernel() const
    {
        return m_kernel;
    }

private:
    Kernel *m_kernel;
    EventDispatcher *m_eventDispatcher;
};

#endif // MODULE_CONTEXT_H
