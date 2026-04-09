#include "prefab_serializer.h"
#include "component_serializer.h"
#include "components.h"
#include "engine/scene/yaml.h"
#include "engine/scene/scene.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
// Simplified serialization logic for specific entities
// Simplified serialization logic for specific entities
static void SerializeEntityData(YAML::Emitter& out, Entity entity)
{
    out << YAML::BeginMap;

    ComponentSerializer::Get().SerializeAll(out, entity);

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
    if (!data["Prefab"])
    {
        return {};
    }

    Entity entity = scene->CreateEntity(data["Prefab"].as<std::string>());
    // Simplified loading logic...
    return entity;
}
} // namespace CHEngine
