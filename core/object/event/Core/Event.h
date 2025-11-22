#ifndef EVENT_H
#define EVENT_H

#include <memory>
#include <string>
#include <typeindex>


// Base class for all events in the system
// Events are used for decoupled communication between modules
class Event
{
public:
    virtual ~Event() = default;

    // Get the event type name for identification
    virtual const char *GetEventType() const = 0;

    // Get type index for runtime type checking
    virtual std::type_index GetTypeIndex() const = 0;

    // Optional: timestamp when event was created
    virtual double GetTimestamp() const
    {
        return m_timestamp;
    }

protected:
    Event() : m_timestamp(0.0)
    {
    }
    explicit Event(double timestamp) : m_timestamp(timestamp)
    {
    }

private:
    double m_timestamp;
};

// Helper macro to define event types with proper type information
#define DEFINE_EVENT_TYPE(EventClass)                                                              \
    const char *GetEventType() const override                                                      \
    {                                                                                              \
        return #EventClass;                                                                        \
    }                                                                                              \
    std::type_index GetTypeIndex() const override                                                  \
    {                                                                                              \
        return std::type_index(typeid(EventClass));                                                \
    }

#endif // EVENT_H
