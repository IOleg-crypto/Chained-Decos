#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/scene/scene.h"
#include "engine/scene/serialization_utils.h"
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
        std::function<void(Entity, Entity)> Copy;
    };

    class ComponentSerializer
    {
    public:
        // Initialize the registry with all component types
        static void Initialize();

        // Register a component serializer (Template implementation below)
        template<typename T>
        static void Register(
            const std::string& yamlKey,
            std::function<void(YAML::Emitter&, T&)> serialize,
            std::function<void(T&, YAML::Node)> deserialize);

        // Register a component serializer using a declarative schema (PropertyBuilder style)
        template<typename T>
        static void Register(const std::string& yamlKey, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema);

        // Serialize all registered components for an entity
        static void SerializeAll(YAML::Emitter& out, Entity entity);

        // Deserialize all registered components from YAML node
        static void DeserializeAll(Entity entity, YAML::Node node);

        // Copy all registered components from source to destination
        static void CopyAll(Entity source, Entity destination);

        // Helper to register UI components
        static void RegisterUIComponents(); 

        // Special cases
        static void SerializeID(YAML::Emitter& out, Entity entity);
        static void SerializeHierarchy(YAML::Emitter& out, Entity entity);
        static void DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask);

    private:
        static std::vector<ComponentSerializerEntry> s_Registry;
    };

    // Template implementation
    template<typename T>
    void ComponentSerializer::Register(const std::string& yamlKey, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema)
    {
        Register<T>(yamlKey,
            [schema](YAML::Emitter& out, T& component) {
                SerializationUtils::PropertyArchive archive(out);
                schema(archive, component);
            },
            [schema](T& component, YAML::Node node) {
                SerializationUtils::PropertyArchive archive(node);
                schema(archive, component);
            }
        );
    }

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
                
                entity.Patch<T>([&](auto& component) {
                    deserialize(component, componentNode);
                });
            }
        };

        entry.Copy = [](Entity source, Entity destination) {
            if (source.HasComponent<T>()) {
                destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
            }
        };
        
        s_Registry.push_back(entry);
    }
}

#endif // CH_COMPONENT_SERIALIZER_H
