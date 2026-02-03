#include "prefab_serializer.h"
#include "engine/scene/scene.h"
#include "components.h"
#include "engine/core/yaml.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
    // Simplified serialization logic for specific entities
    static void SerializeEntityData(YAML::Emitter& out, Entity entity)
    {
        out << YAML::BeginMap;
        
        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap << YAML::Key << "Tag" << YAML::Value << entity.GetComponent<TagComponent>().Tag << YAML::EndMap;
        }

        if (entity.HasComponent<TransformComponent>())
        {
            auto& tc = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap;
            out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
            out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << tc.Scale;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<ModelComponent>())
        {
            auto& mc = entity.GetComponent<ModelComponent>();
            out << YAML::Key << "ModelComponent";
            out << YAML::BeginMap << YAML::Key << "ModelPath" << YAML::Value << mc.ModelPath << YAML::EndMap;
        }

        // Add other components as needed...
        
        out << YAML::EndMap;
    }

    bool PrefabSerializer::Serialize(Entity entity, const std::string& filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Prefab" << YAML::Value << entity.GetComponent<TagComponent>().Tag;
        out << YAML::Key << "RootEntity";
        SerializeEntityData(out, entity);
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
        return true;
    }

    Entity PrefabSerializer::Deserialize(Scene* scene, const std::string& filepath)
    {
        YAML::Node data = YAML::LoadFile(filepath);
        if (!data["Prefab"]) return {};

        Entity entity = scene->CreateEntity(data["Prefab"].as<std::string>());
        // Simplified loading logic...
        return entity;
    }
}
