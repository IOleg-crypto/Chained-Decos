#ifndef LIFETIME_COMPONENT_H
#define LIFETIME_COMPONENT_H

#include <string>
struct LifetimeComponent
{
    float lifetime = 5.0f; // Час життя в секундах
    float timer = 0.0f;    // Поточний час
    bool destroyOnTimeout = true;
};

struct TagComponent
{
    std::string tag; // Для пошуку entities по тегу
};

struct NameComponent
{
    std::string name; // Ім'я entity для дебагу
};

#endif // LIFETIME_COMPONENT_H
