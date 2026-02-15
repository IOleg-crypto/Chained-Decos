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

    // Serializer description for a specific component
    struct ComponentSerializerEntry
    {
        std::string Key;
        std::function<void(YAML::Emitter&, Entity)> Serialize;
        std::function<void(Entity, YAML::Node)> Deserialize;
        std::function<void(Entity, Entity)> Copy;
    };

    class ComponentSerializer
    {
    public:
        // Initialize registry with all component types
        static void Initialize();

        // Register component via declarative schema (PropertyArchive)
        // This is the primary method that automatically creates serialization, deserialization, and copy logic.
        template<typename T>
        static void Register(const std::string& key, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema);

        // Register with custom logic (for complex cases)
        static void RegisterCustom(const ComponentSerializerEntry& entry);

        // Serialize all registered components of an entity
        static void SerializeAll(YAML::Emitter& out, Entity entity);

        // Deserialize all registered components from YAML
        static void DeserializeAll(Entity entity, YAML::Node node);

        // Copy all components from source to destination (cloning)
        static void CopyAll(Entity source, Entity destination);

        // Special cases (ID and hierarchy)
        static void SerializeID(YAML::Emitter& out, Entity entity);
        static void SerializeHierarchy(YAML::Emitter& out, Entity entity);
        static void DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask);

    private:
        static std::vector<ComponentSerializerEntry> s_Registry;
    };

    // Template implementation
    template<typename T>
    void ComponentSerializer::Register(const std::string& key, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema)
    {
        ComponentSerializerEntry entry;
        entry.Key = key;
        
        // Serialization: check for presence and write as a Map
        entry.Serialize = [key, schema](YAML::Emitter& out, Entity entity) {
            if (entity.HasComponent<T>()) {
                out << YAML::Key << key << YAML::Value << YAML::BeginMap;
                SerializationUtils::PropertyArchive archive(out);
                schema(archive, entity.GetComponent<T>());
                out << YAML::EndMap;
            }
        };
        
        // Deserialization: add component and populate with data
        entry.Deserialize = [key, schema](Entity entity, YAML::Node node) {
            if (node[key]) {
                if (!entity.HasComponent<T>())
                    entity.AddComponent<T>();
                
                entity.Patch<T>([&](auto& component) {
                    SerializationUtils::PropertyArchive archive(node[key]);
                    schema(archive, component);
                });
            }
        };

        // Copying: automatic cloning via EnTT
        entry.Copy = [](Entity source, Entity destination) {
            if (source.HasComponent<T>()) {
                destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
            }
        };
        
        RegisterCustom(entry);
    }
}

#endif // CH_COMPONENT_SERIALIZER_H
