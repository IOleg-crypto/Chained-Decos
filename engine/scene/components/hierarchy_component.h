#ifndef CH_HIERARCHY_COMPONENT_H
#define CH_HIERARCHY_COMPONENT_H

#include "entt/entt.hpp"
#include "engine/reflection/reflection_rfl.h"
#include <string>
#include <vector>

namespace Chained
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

} // namespace Chained

#endif // CH_HIERARCHY_COMPONENT_H
