#ifndef HIERARCHY_COMPONENT_H
#define HIERARCHY_COMPONENT_H

#include <entt/entt.hpp>
#include <vector>

namespace CHEngine
{

struct HierarchyComponent
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

struct NameComponent
{
    std::string name;
};

} // namespace CHEngine

#endif // HIERARCHY_COMPONENT_H
