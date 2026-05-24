#ifndef CH_NAVIGATION_COMPONENT_H
#define CH_NAVIGATION_COMPONENT_H

#include <entt/entt.hpp>
#include "engine/core/reflection_rfl.h"

// Now useless
namespace CHEngine
{
struct NavigationComponent
{
    entt::entity Up = ::entt::null;
    entt::entity Down = ::entt::null;
    entt::entity Left = ::entt::null;
    entt::entity Right = ::entt::null;

    bool IsDefaultFocus = false;

    static const char* GetStaticName() { return "NavigationComponent"; }
};

CH_MARK_RFL(NavigationComponent);

} // namespace CHEngine

#endif // CH_NAVIGATION_COMPONENT_H
