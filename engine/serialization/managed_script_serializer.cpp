#include "managed_script_serializer.h"
#include "components/scripting_components.h"
#include "engine/scene/component_registry.h"

namespace Chained
{

void RegisterManagedScriptComponentMetadata()
{
    ComponentMetadata metadata;
    metadata.Name = "Managed Script";
    metadata.SerializationKey = "ManagedScriptComponent";
    metadata.Category = "Scripting";

    metadata.Serialize = [](YAML::Emitter& out, Entity entity) {
        if (!entity.HasComponent<ManagedScriptComponent>())
            return;

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        out << YAML::Key << "ManagedScriptComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;

        for (const auto& s : comp.Scripts)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "ClassName" << YAML::Value << s.ClassName;

            if (!s.Fields.empty())
            {
                out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;
                for (const auto& [fieldName, field] : s.Fields)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name"  << YAML::Value << fieldName;
                    out << YAML::Key << "Type"  << YAML::Value << (int)field.Type;

                    std::visit([&out](auto&& val) {
                        using T = std::decay_t<decltype(val)>;
                        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int> ||
                                      std::is_same_v<T, bool>  || std::is_same_v<T, std::string> ||
                                      std::is_same_v<T, uint64_t>)
                        {
                            out << YAML::Key << "Value" << YAML::Value << val;
                        }
                        else if constexpr (std::is_same_v<T, glm::vec2>)
                        {
                            out << YAML::Key << "Value" << YAML::Value
                                << YAML::Flow << YAML::BeginSeq << val.x << val.y << YAML::EndSeq;
                        }
                        else if constexpr (std::is_same_v<T, glm::vec3>)
                        {
                            out << YAML::Key << "Value" << YAML::Value
                                << YAML::Flow << YAML::BeginSeq << val.x << val.y << val.z << YAML::EndSeq;
                        }
                        else if constexpr (std::is_same_v<T, glm::vec4>)
                        {
                            out << YAML::Key << "Value" << YAML::Value
                                << YAML::Flow << YAML::BeginSeq << val.x << val.y << val.z << val.w << YAML::EndSeq;
                        }
                        else if constexpr (std::is_same_v<T, Chained::Color>)
                        {
                            out << YAML::Key << "Value" << YAML::Value
                                << YAML::Flow << YAML::BeginSeq
                                << (int)val.r << (int)val.g << (int)val.b << (int)val.a
                                << YAML::EndSeq;
                        }
                    }, field.Value);

                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }

            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;
    };

    metadata.Deserialize = [](Entity entity, YAML::Node node) {
        if (!node["ManagedScriptComponent"])
            return;

        if (!entity.HasComponent<ManagedScriptComponent>())
            entity.AddComponent<ManagedScriptComponent>();

        auto& comp = entity.GetComponent<ManagedScriptComponent>();
        comp.Scripts.clear();

        auto scriptsNode = node["ManagedScriptComponent"]["Scripts"];
        if (!scriptsNode || !scriptsNode.IsSequence())
            return;

        for (auto s : scriptsNode)
        {
            ManagedScriptInstance inst;
            if (s["ClassName"])
                inst.ClassName = s["ClassName"].as<std::string>();

            if (s["Fields"] && s["Fields"].IsSequence())
            {
                for (auto fieldNode : s["Fields"])
                {
                    if (!fieldNode["Name"] || !fieldNode["Type"])
                        continue;

                    std::string name   = fieldNode["Name"].as<std::string>();
                    auto        type   = (ScriptFieldType)fieldNode["Type"].as<int>(0);
                    auto        valN   = fieldNode["Value"];

                    ScriptField field;
                    field.Type = type;
                    field.Name = name;

                    if (valN)
                    {
                        switch (type)
                        {
                            case ScriptFieldType::Float:
                                field.Value = valN.as<float>(0.0f);     break;
                            case ScriptFieldType::Int:
                                field.Value = valN.as<int>(0);          break;
                            case ScriptFieldType::Bool:
                                field.Value = valN.as<bool>(false);     break;
                            case ScriptFieldType::String:
                                field.Value = valN.as<std::string>(""); break;
                            case ScriptFieldType::Entity:
                                field.Value = valN.as<uint64_t>(0);     break;
                            case ScriptFieldType::Vec2:
                                field.Value = (valN.IsSequence() && valN.size() >= 2)
                                    ? glm::vec2(valN[0].as<float>(), valN[1].as<float>())
                                    : glm::vec2(0.0f);
                                break;
                            case ScriptFieldType::Vec3:
                                field.Value = (valN.IsSequence() && valN.size() >= 3)
                                    ? glm::vec3(valN[0].as<float>(), valN[1].as<float>(), valN[2].as<float>())
                                    : glm::vec3(0.0f);
                                break;
                            case ScriptFieldType::Vec4:
                                field.Value = (valN.IsSequence() && valN.size() >= 4)
                                    ? glm::vec4(valN[0].as<float>(), valN[1].as<float>(),
                                                valN[2].as<float>(), valN[3].as<float>())
                                    : glm::vec4(0.0f);
                                break;
                            case ScriptFieldType::Color:
                                field.Value = (valN.IsSequence() && valN.size() >= 4)
                                    ? Chained::Color{(unsigned char)valN[0].as<int>(),
                                                     (unsigned char)valN[1].as<int>(),
                                                     (unsigned char)valN[2].as<int>(),
                                                     (unsigned char)valN[3].as<int>()}
                                    : Chained::Color{255, 255, 255, 255};
                                break;
                            default:
                                field.Value = 0.0f; break;
                        }
                    }

                    inst.Fields[name] = std::move(field);
                }
            }

            comp.Scripts.push_back(std::move(inst));
        }
    };

    metadata.Copy = [](Entity src, Entity dst) {
        if (src.HasComponent<ManagedScriptComponent>())
            dst.AddOrReplaceComponent<ManagedScriptComponent>(
                src.GetComponent<ManagedScriptComponent>().ClonePersistent());
    };
    metadata.Has    = [](Entity e)       { return e.HasComponent<ManagedScriptComponent>(); };
    metadata.GetAll = [](class Scene* s) {
        std::vector<uint64_t> ids;
        for (auto ent : s->GetRegistry().view<ManagedScriptComponent>())
            ids.push_back((uint64_t)(uint32_t)ent);
        return ids;
    };
    metadata.Add    = [](Entity e) {
        if (!e.HasComponent<ManagedScriptComponent>())
            e.AddComponent<ManagedScriptComponent>();
    };
    metadata.Remove = [](Entity e) {
        if (e.HasComponent<ManagedScriptComponent>())
            e.RemoveComponent<ManagedScriptComponent>();
    };

    ComponentRegistry::Register(entt::type_hash<ManagedScriptComponent>::value(), metadata);
}

} // namespace Chained