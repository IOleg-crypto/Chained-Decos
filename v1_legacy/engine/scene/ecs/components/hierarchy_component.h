#ifndef CD_SCENE_ECS_COMPONENTS_HIERARCHY_COMPONENT_H
#define CD_SCENE_ECS_COMPONENTS_HIERARCHY_COMPONENT_H

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

#endif // CD_SCENE_ECS_COMPONENTS_HIERARCHY_COMPONENT_H
