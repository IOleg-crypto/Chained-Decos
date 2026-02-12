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

    // Опис серіалізатора для конкретного компонента
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
        // Ініціалізація реєстру всіма типами компонентів
        static void Initialize();

        // Реєстрація компонента через декларативну схему (PropertyArchive)
        // Це основний метод, який автоматично створює логіку серіалізації, десеріалізації та копіювання.
        template<typename T>
        static void Register(const std::string& key, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema);

        // Реєстрація з кастомною логікою (для складних випадків)
        static void RegisterCustom(const ComponentSerializerEntry& entry);

        // Серіалізація всіх зареєстрованих компонентів сутності
        static void SerializeAll(YAML::Emitter& out, Entity entity);

        // Десеріалізація всіх зареєстрованих компонентів з YAML
        static void DeserializeAll(Entity entity, YAML::Node node);

        // Копіювання всіх компонентів від джерела до цілі (клонування)
        static void CopyAll(Entity source, Entity destination);

        // Спеціальні випадки (ID та ієрархія)
        static void SerializeID(YAML::Emitter& out, Entity entity);
        static void SerializeHierarchy(YAML::Emitter& out, Entity entity);
        static void DeserializeHierarchyTask(Entity entity, YAML::Node node, HierarchyTask& outTask);

    private:
        static std::vector<ComponentSerializerEntry> s_Registry;
    };

    // Реалізація шаблонів
    template<typename T>
    void ComponentSerializer::Register(const std::string& key, std::function<void(SerializationUtils::PropertyArchive&, T&)> schema)
    {
        ComponentSerializerEntry entry;
        entry.Key = key;
        
        // Серіалізація: перевіряємо наявність і записуємо як Map
        entry.Serialize = [key, schema](YAML::Emitter& out, Entity entity) {
            if (entity.HasComponent<T>()) {
                out << YAML::Key << key << YAML::Value << YAML::BeginMap;
                SerializationUtils::PropertyArchive archive(out);
                schema(archive, entity.GetComponent<T>());
                out << YAML::EndMap;
            }
        };
        
        // Десеріалізація: додаємо компонент та наповнюємо даними
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

        // Копіювання: автоматичне клонування через EnTT
        entry.Copy = [](Entity source, Entity destination) {
            if (source.HasComponent<T>()) {
                destination.AddOrReplaceComponent<T>(source.GetComponent<T>());
            }
        };
        
        RegisterCustom(entry);
    }
}

#endif // CH_COMPONENT_SERIALIZER_H
