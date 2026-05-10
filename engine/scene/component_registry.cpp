#include "component_registry.h"

namespace CHEngine
{
    std::unordered_map<::entt::id_type, ComponentMetadata> ComponentRegistry::s_Registry;

    void ComponentRegistry::Register(::entt::id_type typeId, const ComponentMetadata& metadata)
    {
        s_Registry[typeId] = metadata;
    }
}
