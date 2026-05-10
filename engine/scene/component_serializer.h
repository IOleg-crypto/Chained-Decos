#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/scene/component_registry.h"
#include "engine/scene/serialization_utils.h"
#include "engine/core/engine_service.h"
#include "engine/scene/scene.h"
#include "entt/entt.hpp"
#include <functional>

#include <vector>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
// Describes how one component is serialized, deserialized, and copied.
struct ComponentSerializerEntry
{
    std::string Key;
    std::function<void(YAML::Emitter&, Entity)> Serialize;
    std::function<void(Entity, YAML::Node)> Deserialize;
    std::function<void(Entity, Entity)> Copy;
};

// Central registry for component serializers used by scene save/load/copy.
class ComponentSerializer : public EngineService
{
public:
    ComponentSerializer();
    virtual ~ComponentSerializer() override;

public:
    virtual void OnInit() override;
    virtual void OnShutdown() override;

protected:

private:
    void RegisterCoreComponents();
    void RegisterPhysicsComponents();
    void RegisterAudioComponents();
    void RegisterGameplayComponents();
    void RegisterUIComponents();
    void RegisterScriptingComponents();

public:
    // Registers a component via a PropertyArchive schema.
    // This path automatically creates serialization, deserialization, and copy logic.
    template <typename T>
    void Register(const std::string& key, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema);

    // Registers a component by calling T::Reflect or T::Serialize-style code.
    template <typename T> void Register(const std::string& key);

    // Registers a component using T::GetStaticName() as the key.
    template <typename T> void Register();

    // Registers custom serialization logic for special cases.
    void RegisterCustom(const ComponentSerializerEntry& entry);

    // Serializes all registered components owned by an entity.
    void SerializeAll(YAML::Emitter& out, Entity entity);

    // Deserializes all registered components from YAML.
    void DeserializeAll(Entity entity, YAML::Node node);

    // Copies all registered components from source to destination.
    void CopyAll(Entity source, Entity destination);

    // Serializes the ID component separately.
    void SerializeID(YAML::Emitter& out, Entity entity);


private:
    std::vector<ComponentSerializerEntry> m_Registry;
};

// Template implementation
template <typename T>
void ComponentSerializer::Register(const std::string& key,
                                   std::function<void(SerializationUtils::PropertyArchive&, T&)> schema)
{
    ComponentSerializerEntry entry;
    entry.Key = key;

    // Serialization: check for presence and write as a Map
    entry.Serialize = [key, schema](YAML::Emitter& out, Entity entity) {
        if (entity.HasComponent<T>())
        {
            out << YAML::Key << key << YAML::Value << YAML::BeginMap;
            SerializationUtils::PropertyArchive archive(out);
            schema(archive, entity.GetComponent<T>());
            out << YAML::EndMap;
        }
    };

    // Deserialization: add component and populate with data
    entry.Deserialize = [key, schema](Entity entity, YAML::Node node) {
        if (node[key])
        {
            if (!entity.HasComponent<T>())
            {
                entity.AddComponent<T>();
            }

            entity.Patch<T>([&](auto& component) {
                SerializationUtils::PropertyArchive archive(node[key]);
                schema(archive, component);
            });
        }
    };

    // Copying: automatic cloning via EnTT
    entry.Copy = [](Entity source, Entity destination) {
        if (source.HasComponent<T>())
        {
            destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
        }
    };

    // Bridge to the new ComponentRegistry metadata system
    if (!ComponentRegistry::Exists(entt::type_hash<T>::value()))
    {
        ComponentMetadata metadata;
        metadata.Name = key; // Use serialization key as fallback name
        metadata.SerializationKey = key;
        metadata.Serialize = entry.Serialize;
        metadata.Deserialize = entry.Deserialize;
        metadata.Copy = entry.Copy;
        metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
        metadata.GetAll = [](class Scene* s) { 
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<T>()) ids.push_back((uint64_t)(uint32_t)ent);
            return ids;
        };
        metadata.Add = [](Entity e) { if (!e.HasComponent<T>()) e.AddComponent<T>(); };
        metadata.Remove = [](Entity e) { if (e.HasComponent<T>()) e.RemoveComponent<T>(); };
        ComponentRegistry::Register(entt::type_hash<T>::value(), metadata);
    }
    else
    {
        auto& metadata = ComponentRegistry::GetMetadataMutable(entt::type_hash<T>::value());
        metadata.SerializationKey = key;
        metadata.Serialize = entry.Serialize;
        metadata.Deserialize = entry.Deserialize;
        metadata.Copy = entry.Copy;
        metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
        metadata.GetAll = [](class Scene* s) { 
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<T>()) ids.push_back((uint64_t)(uint32_t)ent);
            return ids;
        };
        metadata.Add = [](Entity e) { if (!e.HasComponent<T>()) e.AddComponent<T>(); };
        metadata.Remove = [](Entity e) { if (e.HasComponent<T>()) e.RemoveComponent<T>(); };
    }

    RegisterCustom(entry);
}

template <typename T> void ComponentSerializer::Register(const std::string& key)
{
    ComponentSerializerEntry entry;
    entry.Key = key;

    entry.Serialize = [key](YAML::Emitter& out, Entity entity) {
        if (entity.HasComponent<T>())
        {
            out << YAML::Key << key << YAML::Value << YAML::BeginMap;
            SerializationUtils::PropertyArchive archive(out);
            CHEngine::Properties props(archive);
            entity.GetComponent<T>().Reflect(props);
            out << YAML::EndMap;
        }
    };

    entry.Deserialize = [key](Entity entity, YAML::Node node) {
        if (node[key])
        {
            if (!entity.HasComponent<T>())
            {
                entity.AddComponent<T>();
            }
            entity.Patch<T>([&](auto& component) {
                SerializationUtils::PropertyArchive archive(node[key]);
                CHEngine::Properties props(archive);
                component.Reflect(props);
            });
        }
    };

    entry.Copy = [](Entity source, Entity destination) {
        if (source.HasComponent<T>())
        {
            destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
        }
    };

    // Bridge to the new ComponentRegistry metadata system
    if (!ComponentRegistry::Exists(entt::type_hash<T>::value()))
    {
        ComponentMetadata metadata;
        metadata.Name = key; // Use serialization key as fallback name
        metadata.SerializationKey = key;
        metadata.Serialize = entry.Serialize;
        metadata.Deserialize = entry.Deserialize;
        metadata.Copy = entry.Copy;
        metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
        metadata.GetAll = [](class Scene* s) { 
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<T>()) ids.push_back((uint64_t)(uint32_t)ent);
            return ids;
        };
        metadata.Add = [](Entity e) { if (!e.HasComponent<T>()) e.AddComponent<T>(); };
        metadata.Remove = [](Entity e) { if (e.HasComponent<T>()) e.RemoveComponent<T>(); };
        ComponentRegistry::Register(entt::type_hash<T>::value(), metadata);
    }
    else
    {
        auto& metadata = ComponentRegistry::GetMetadataMutable(entt::type_hash<T>::value());
        metadata.SerializationKey = key;
        metadata.Serialize = entry.Serialize;
        metadata.Deserialize = entry.Deserialize;
        metadata.Copy = entry.Copy;
        metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
        metadata.GetAll = [](class Scene* s) { 
            std::vector<uint64_t> ids;
            for (auto ent : s->GetRegistry().view<T>()) ids.push_back((uint64_t)(uint32_t)ent);
            return ids;
        };
        metadata.Add = [](Entity e) { if (!e.HasComponent<T>()) e.AddComponent<T>(); };
        metadata.Remove = [](Entity e) { if (e.HasComponent<T>()) e.RemoveComponent<T>(); };
    }

    RegisterCustom(entry);
}

template <typename T> void ComponentSerializer::Register()
{
    Register<T>(T::GetStaticName());
}

} // namespace CHEngine

#endif // CH_COMPONENT_SERIALIZER_H
