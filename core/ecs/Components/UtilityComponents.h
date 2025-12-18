#ifndef LIFETIME_COMPONENT_H
#define LIFETIME_COMPONENT_H

#include <string>
struct LifetimeComponent
{
    float lifetime = 5.0f; // Lifetime in seconds
    float timer = 0.0f;    // Current time
    bool destroyOnTimeout = true;
};

struct TagComponent
{
    std::string tag; // For finding entities by tag
};

struct NameComponent
{
    std::string name; // Entity name for debugging
};

#endif // LIFETIME_COMPONENT_H
