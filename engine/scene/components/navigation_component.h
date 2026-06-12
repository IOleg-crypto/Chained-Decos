#ifndef CH_NAVIGATION_COMPONENT_H
#define CH_NAVIGATION_COMPONENT_H

#include <entt/entt.hpp>
#include "engine/reflection/reflection_rfl.h"

// Now useless
namespace Chained
{
struct NavigationComponent
{
    entt::entity Up = entt::null;
    entt::entity Down = entt::null;
    entt::entity Left = entt::null;
    entt::entity Right = entt::null;

    bool IsDefaultFocus = false;

    static const char* GetStaticName() { return "NavigationComponent"; }
};

CH_MARK_RFL(NavigationComponent);

} // namespace Chained

#endif // CH_NAVIGATION_COMPONENT_H
