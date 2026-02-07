#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include "script_registry.h"
#include "components/hierarchy_component.h"
#include "components/id_component.h"

namespace CHEngine
{
    std::vector<ComponentSerializerEntry> ComponentSerializer::s_Registry;

    void ComponentSerializer::Initialize()
    {
        s_Registry.clear();

        RegisterGraphicsComponents();
        RegisterPhysicsComponents();
        RegisterAudioComponents();
        RegisterGameplayComponents();
        RegisterUIComponents();
    }

    void ComponentSerializer::SerializeAll(YAML::Emitter& out, Entity entity)
    {
        for (auto& entry : s_Registry)
        {
            entry.Serialize(out, entity);
        }
        
        SerializeHierarchy(out, entity);
    }

    void ComponentSerializer::DeserializeAll(Entity entity, YAML::Node node)
    {
        for (const auto& entry : s_Registry)
        {
            if (entry.Deserialize)
            {
                entry.Deserialize(entity, node);
            }
        }
    }

    void ComponentSerializer::CopyAll(Entity source, Entity destination)
    {
        for (const auto& entry : s_Registry)
        {
            if (entry.Copy)
            {
                entry.Copy(source, destination);
            }
        }
    }
    void ComponentSerializer::SerializeID(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<IDComponent>())
            out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entity.GetComponent<IDComponent>().ID;
        else
            out << YAML::Key << "Entity" << YAML::Value << 0;
    }

    void ComponentSerializer::SerializeHierarchy(YAML::Emitter& out, Entity entity)
    {
        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& hc = entity.GetComponent<HierarchyComponent>();
            out << YAML::Key << "Hierarchy";
            out << YAML::BeginMap;

            uint64_t parentUUID = 0;
            if (hc.Parent != entt::null)
            {
                Entity parent{hc.Parent, entity.GetScene()};
                if (parent.HasComponent<IDComponent>())
                    parentUUID = (uint64_t)parent.GetComponent<IDComponent>().ID;
            }
            out << YAML::Key << "Parent" << YAML::Value << parentUUID;

            out << YAML::Key << "Children" << YAML::BeginSeq;
            for (auto childHandle : hc.Children)
            {
                Entity child{childHandle, entity.GetScene()};
                if (child.HasComponent<IDComponent>())
                    out << (uint64_t)child.GetComponent<IDComponent>().ID;
            }
            out << YAML::EndSeq;

            out << YAML::EndMap;
        }
    }

    void ComponentSerializer::DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask)
    {
        auto hierarchyNode = node["Hierarchy"];
        if (hierarchyNode)
        {
            outTask.entity = entity;
            outTask.parent = hierarchyNode["Parent"] ? hierarchyNode["Parent"].as<uint64_t>() : 0;
            if (hierarchyNode["Children"])
            {
                for (auto child : hierarchyNode["Children"])
                    outTask.children.push_back(child.as<uint64_t>());
            }
        }
    }

} // namespace CHEngine
