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

    static const char* GetStaticName() { return "HierarchyComponent"; }

    
    struct UI
    {
        UIMeta Parent = {.Tooltip = "ID батьківської сутності (entt::entity)"};
        // Children typically not edited directly via properties, but let's leave it without UIMeta or add a simple one
    };
};

CH_MARK_RFL(HierarchyComponent);

struct NameComponent
{
    std::string Name;

    static const char* GetStaticName() { return "NameComponent"; }

    
    struct UI
    {
        UIMeta Name = {.Tooltip = "Ім'я сутності в ієрархії"};
    };
};

CH_MARK_RFL(NameComponent);

} // namespace Chained

#endif // CH_HIERARCHY_COMPONENT_H
