#ifndef CH_NAVIGATION_COMPONENT_H
#define CH_NAVIGATION_COMPONENT_H

#include <entt/entt.hpp>

namespace CHEngine
{
    struct NavigationComponent
    {
        entt::entity Up = entt::null;
        entt::entity Down = entt::null;
        entt::entity Left = entt::null;
        entt::entity Right = entt::null;

        bool IsDefaultFocus = false;

        NavigationComponent() = default;
        NavigationComponent(const NavigationComponent&) = default;
    };
}

#endif // CH_NAVIGATION_COMPONENT_H
