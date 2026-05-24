#ifndef CH_COMPONENT_SERIALIZER_H
#define CH_COMPONENT_SERIALIZER_H

#include "engine/core/engine_service.h"
#include "engine/scene/component_registry.h"
#include "engine/scene/scene.h"
#include "engine/scene/serialization.h"
#include "entt/entt.hpp"
#include <functional>

#include <vector>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
// Central registry for component serializers used by scene save/load/copy.
class ComponentSerializer : public EngineService
{
public:
    ComponentSerializer();
    virtual ~ComponentSerializer() override;

public:
    virtual void OnInit() override;
    virtual void OnShutdown() override;

public:
    // Registers a component via a PropertyArchive schema.
    // This path automatically creates serialization, deserialization, and copy logic.
    template <typename T>
    void Register(const std::string& key, std::function<void(Serialization::PropertyArchive&, T&)> schema);

    // Registers a component by calling T::Reflect or T::Serialize-style code.
    template <typename T> void Register(const std::string& key);

    // Registers a component using T::GetStaticName() as the key.
    template <typename T> void Register();

    // Serializes all registered components owned by an entity.
    void SerializeAll(YAML::Emitter& out, Entity entity);

    // Deserializes all registered components from YAML.
    void DeserializeAll(Entity entity, YAML::Node node);

    // Copies all registered components from source to destination.
    void CopyAll(Entity source, Entity destination);

    // Serializes the ID component separately.
    void SerializeID(YAML::Emitter& out, Entity entity);
};

// Template implementation
template <typename T>
void ComponentSerializer::Register(const std::string& key,
                                   std::function<void(Serialization::PropertyArchive&, T&)> schema)
{
    ComponentMetadata metadata;
    metadata.Name = key;
    metadata.SerializationKey = key;
    metadata.Serialize = [key, schema](YAML::Emitter& out, Entity entity) {
        if (entity.HasComponent<T>())
        {
            out << YAML::Key << key << YAML::Value << YAML::BeginMap;
            Serialization::PropertyArchive archive(out);
            schema(archive, entity.GetComponent<T>());
            out << YAML::EndMap;
        }
    };
    metadata.Deserialize = [key, schema](Entity entity, YAML::Node node) {
        if (node[key])
        {
            if (!entity.HasComponent<T>())
            {
                entity.AddComponent<T>();
            }

            entity.Patch<T>([&](auto& component) {
                Serialization::PropertyArchive archive(node[key]);
                schema(archive, component);
            });
        }
    };
    metadata.Copy = [](Entity source, Entity destination) {
        if (source.HasComponent<T>())
        {
            destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
        }
    };
    metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
    metadata.GetAll = [](class Scene* s) {
        std::vector<uint64_t> ids;
        for (auto ent : s->GetRegistry().view<T>())
        {
            ids.push_back((uint64_t)(uint32_t)ent);
        }
        return ids;
    };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>())
        {
            e.AddComponent<T>();
        }
    };
    metadata.Remove = [](Entity e) {
        if (e.HasComponent<T>())
        {
            e.RemoveComponent<T>();
        }
    };

    ComponentRegistry::Register(::entt::type_hash<T>::value(), metadata);
}

template <typename T> void ComponentSerializer::Register(const std::string& key)
{
    ComponentMetadata metadata;
    metadata.Name = key;
    metadata.SerializationKey = key;
    metadata.Serialize = [key](YAML::Emitter& out, Entity entity) {
        if (entity.HasComponent<T>())
        {
            out << YAML::Key << key << YAML::Value << YAML::BeginMap;
            Serialization::PropertyArchive archive(out);
            CHEngine::Properties props(archive);
            if constexpr (is_rfl_component<T>::value)
                ReflectFromRfl(entity.GetComponent<T>(), props);
            else
                entity.GetComponent<T>().Reflect(props);
            out << YAML::EndMap;
        }
    };
    metadata.Deserialize = [key](Entity entity, YAML::Node node) {
        if (node[key])
        {
            if (!entity.HasComponent<T>())
            {
                entity.AddComponent<T>();
            }
            entity.Patch<T>([&](auto& component) {
                Serialization::PropertyArchive archive(node[key]);
                CHEngine::Properties props(archive);
                if constexpr (is_rfl_component<T>::value)
                    ReflectFromRfl(component, props);
                else
                    component.Reflect(props);
            });
        }
    };
    metadata.Copy = [](Entity source, Entity destination) {
        if (source.HasComponent<T>())
        {
            destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
        }
    };
    metadata.Has = [](Entity e) { return e.HasComponent<T>(); };
    metadata.GetAll = [](class Scene* s) {
        std::vector<uint64_t> ids;
        for (auto ent : s->GetRegistry().view<T>())
        {
            ids.push_back((uint64_t)(uint32_t)ent);
        }
        return ids;
    };
    metadata.Add = [](Entity e) {
        if (!e.HasComponent<T>())
        {
            e.AddComponent<T>();
        }
    };
    metadata.Remove = [](Entity e) {
        if (e.HasComponent<T>())
        {
            e.RemoveComponent<T>();
        }
    };

    ComponentRegistry::Register(::entt::type_hash<T>::value(), metadata);
}

template <typename T> void ComponentSerializer::Register()
{
    Register<T>(T::GetStaticName());
}

} // namespace CHEngine

#endif // CH_COMPONENT_SERIALIZER_H
