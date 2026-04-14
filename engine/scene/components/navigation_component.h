#ifndef CH_NAVIGATION_COMPONENT_H
#define CH_NAVIGATION_COMPONENT_H

#include <entt/entt.hpp>
#include "engine/core/reflection.h"

namespace CHEngine
{
struct NavigationComponent
{
    entt::entity Up = entt::null;
    entt::entity Down = entt::null;
    entt::entity Left = entt::null;
    entt::entity Right = entt::null;

    bool IsDefaultFocus = false;

    CH_REFLECT_BEGIN(NavigationComponent)
        props.Property("Is Default Focus", IsDefaultFocus);
        // Navigation targets (entt::entity) are usually handled by Entity picker in UI
    CH_REFLECT_END()
};
} // namespace CHEngine

#endif // CH_NAVIGATION_COMPONENT_H
