#include "prefab_serializer.h"
#include "component_serializer.h"
#include "components.h"
#include "engine/scene/yaml.h"
#include "engine/scene/hierarchy_serializer.h"
#include "engine/scene/scene.h"
#include "engine/core/service_locator.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
static void SerializeEntityRecursive(YAML::Emitter& out, Entity entity)
{
    out << YAML::BeginMap;
    // We must manually serialize the ID component early or rely on ComponentSerializer
    ServiceLocator::Get<ComponentSerializer>().SerializeAll(out, entity);
    out << YAML::EndMap;

    if (entity.HasComponent<HierarchyComponent>())
    {
        auto& hc = entity.GetComponent<HierarchyComponent>();
        for (auto childHandle : hc.Children)
        {
            Entity child = {childHandle, &entity.GetRegistry()};
            if (child)
            {
                SerializeEntityRecursive(out, child);
            }
        }
    }
}

bool PrefabSerializer::Serialize(Entity entity, const std::string& filepath)
{
    if (!entity) return false;

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Prefab" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
    out << YAML::Key << "RootEntity" << YAML::Value << (uint64_t)entity.GetUUID();
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    
    SerializeEntityRecursive(out, entity);
    
    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filepath);
    fout << out.c_str();
    return true;
}

Entity PrefabSerializer::Deserialize(Scene* scene, const std::string& filepath)
{
    YAML::Node data;
    try {
        data = YAML::LoadFile(filepath);
    } catch (...) {
        CH_CORE_ERROR("PrefabSerializer: Failed to load prefab file '{}'", filepath);
        return {};
    }

    if (!data["Prefab"] || !data["Entities"])
    {
        CH_CORE_ERROR("PrefabSerializer: Invalid prefab file format '{}'", filepath);
        return {};
    }

    auto entitiesNode = data["Entities"];

    std::unordered_map<uint64_t, Entity> remapTable;
    std::vector<Entity> createdEntities;
    std::vector<HierarchyTask> hierarchyTasks;

    // Step 1: Create all entities first
    for (auto entityNode : entitiesNode)
    {
        if (!entityNode["Entity"]) continue;
        
        uint64_t oldUUID = entityNode["Entity"].as<uint64_t>();
        std::string tag = "Entity";
        if (entityNode["TagComponent"] && entityNode["TagComponent"]["Tag"])
            tag = entityNode["TagComponent"]["Tag"].as<std::string>();

        Entity newEntity = scene->CreateEntity(tag);
        remapTable[oldUUID] = newEntity;
        createdEntities.push_back(newEntity);
    }

    // Step 2: Deserialize components and collect hierarchy tasks
    int idx = 0;
    for (auto entityNode : entitiesNode)
    {
        if (idx >= createdEntities.size()) break;
        
        Entity entity = createdEntities[idx++];
        ServiceLocator::Get<ComponentSerializer>().DeserializeAll(entity, entityNode);
        
        HierarchyTask task;
        HierarchySerializer::DeserializeTask(entity, entityNode, task);
        if (task.parent != 0 || !task.children.empty())
        {
            hierarchyTasks.push_back(task);
        }
    }

    // Step 3: Reconstruct hierarchy using the remap table
    for (const auto& task : hierarchyTasks)
    {
        Entity entity = task.entity;
        auto& hc = entity.AddOrReplaceComponent<HierarchyComponent>();
        
        // Fix Parent
        if (task.parent != 0 && remapTable.count(task.parent))
        {
            hc.Parent = (entt::entity)remapTable[task.parent];
        }
        else
        {
            hc.Parent = entt::null;
        }

        // Fix Children
        hc.Children.clear();
        for (uint64_t oldChildID : task.children)
        {
            if (remapTable.count(oldChildID))
            {
                hc.Children.push_back((entt::entity)remapTable[oldChildID]);
            }
        }
    }

    // Step 4: Find and return the root entity
    uint64_t rootOldUUID = data["RootEntity"].as<uint64_t>();
    if (remapTable.count(rootOldUUID))
        return remapTable[rootOldUUID];

    return createdEntities.empty() ? Entity{} : createdEntities[0];
}
} // namespace CHEngine
