#ifndef CH_HIERARCHY_COMPONENT_H
#define CH_HIERARCHY_COMPONENT_H

#include "entt/entt.hpp"
#include "engine/core/reflection_rfl.h"
#include <string>
#include <vector>

namespace CHEngine
{
struct HierarchyComponent
{
    ::entt::entity Parent = ::entt::null;
    std::vector<::entt::entity> Children;
};

CH_MARK_RFL(HierarchyComponent);

struct NameComponent
{
    std::string Name;


};

CH_MARK_RFL(NameComponent);

} // namespace CHEngine

#endif // CH_HIERARCHY_COMPONENT_H
