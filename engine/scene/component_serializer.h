#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/scene/entity.h"
#include <yaml-cpp/yaml.h>
#include <functional>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
    struct HierarchyTask
    {
        Entity entity;
        uint64_t parent;
        std::vector<uint64_t> children;
    };

    // Declarative serialization registry
    struct ComponentSerializerEntry
    {
        std::string YamlKey;
        std::function<void(YAML::Emitter&, Entity)> Serialize;
        std::function<void(Entity, YAML::Node)> Deserialize;
    };

    class ComponentSerializer
    {
    public:
        // Initialize the registry with all component types
        static void Init();

        // Register a component serializer
        template<typename T>
        static void Register(
            const std::string& yamlKey,
            std::function<void(YAML::Emitter&, T&)> serialize,
            std::function<void(T&, YAML::Node)> deserialize);

        // Serialize all registered components for an entity
        static void SerializeAll(YAML::Emitter& out, Entity entity);

        // Deserialize all registered components from YAML node
        static void DeserializeAll(Entity entity, YAML::Node node);

        // Special cases that need custom handling
        static void SerializeID(YAML::Emitter& out, Entity entity);
        static void SerializeHierarchy(YAML::Emitter& out, Entity entity);
        static void DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask);

    private:
        static std::vector<ComponentSerializerEntry> s_Registry;
    };

    // Template implementation
    template<typename T>
    void ComponentSerializer::Register(
        const std::string& yamlKey,
        std::function<void(YAML::Emitter&, T&)> serialize,
        std::function<void(T&, YAML::Node)> deserialize)
    {
        ComponentSerializerEntry entry;
        entry.YamlKey = yamlKey;
        
        // Wrap to check HasComponent and get/add component
        entry.Serialize = [yamlKey, serialize](YAML::Emitter& out, Entity entity) {
            if (entity.HasComponent<T>()) {
                out << YAML::Key << yamlKey;
                out << YAML::BeginMap;
                serialize(out, entity.GetComponent<T>());
                out << YAML::EndMap;
            }
        };
        
        entry.Deserialize = [yamlKey, deserialize](Entity entity, YAML::Node node) {
            auto componentNode = node[yamlKey];
            if (componentNode) {
                if (!entity.HasComponent<T>())
                    entity.AddComponent<T>();
                deserialize(entity.GetComponent<T>(), componentNode);
            }
        };
        
        s_Registry.push_back(entry);
    }
}

#endif // CH_COMPONENT_SERIALIZER_H
