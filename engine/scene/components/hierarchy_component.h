#ifndef CH_HIERARCHY_COMPONENT_H
#define CH_HIERARCHY_COMPONENT_H

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace CHEngine
{
struct HierarchyComponent
{
    entt::entity Parent = entt::null;
    std::vector<entt::entity> Children;

    HierarchyComponent() = default;
    HierarchyComponent(const HierarchyComponent &) = default;
};

struct NameComponent
{
    std::string Name;

    NameComponent() = default;
    NameComponent(const std::string &name) : Name(name)
    {
    }
};
} // namespace CHEngine

#endif // CH_HIERARCHY_COMPONENT_H
