#ifndef CH_SCRIPTING_SERIALIZATION_H
#define CH_SCRIPTING_SERIALIZATION_H

#include "components/scripting_components.h"
#include "engine/scene/yaml.h"
#include <yaml-cpp/yaml.h>

namespace YAML
{
template <> struct convert<CHEngine::ManagedScriptInstance>
{
    static Node encode(const CHEngine::ManagedScriptInstance& rhs)
    {
        Node node;
        node["ClassName"] = rhs.ClassName;
        if (!rhs.Fields.empty())
        {
            Node fields;
            for (const auto& [name, field] : rhs.Fields)
            {
                Node f;
                f["Name"] = field.Name;
                f["Type"] = (int)field.Type;
                std::visit([&](auto&& arg) { f["Value"] = YAML::Node(arg); }, field.Value);
                fields.push_back(f);
            }
            node["Fields"] = fields;
        }
        return node;
    }

    static bool decode(const Node& node, CHEngine::ManagedScriptInstance& rhs)
    {
        if (node.IsScalar())
        {
            rhs.ClassName = node.as<std::string>();
            return true;
        }

        if (!node.IsMap())
        {
            return false;
        }

        if (node["ClassName"])
        {
            rhs.ClassName = node["ClassName"].as<std::string>();
        }

        if (node["Fields"] && node["Fields"].IsSequence())
        {
            for (auto f : node["Fields"])
            {
                CHEngine::ScriptField field;
                if (f["Name"])
                {
                    field.Name = f["Name"].as<std::string>();
                }
                if (f["Type"])
                {
                    field.Type = (CHEngine::ScriptFieldType)f["Type"].as<int>();
                }

                if (f["Value"])
                {
                    switch (field.Type)
                    {
                    case CHEngine::ScriptFieldType::Float:
                        field.Value = f["Value"].as<float>();
                        break;
                    case CHEngine::ScriptFieldType::Int:
                        field.Value = f["Value"].as<int>();
                        break;
                    case CHEngine::ScriptFieldType::Bool:
                        field.Value = f["Value"].as<bool>();
                        break;
                    case CHEngine::ScriptFieldType::String:
                        field.Value = f["Value"].as<std::string>();
                        break;
                    case CHEngine::ScriptFieldType::Vec2:
                        field.Value = f["Value"].as<glm::vec2>();
                        break;
                    case CHEngine::ScriptFieldType::Vec3:
                        field.Value = f["Value"].as<glm::vec3>();
                        break;
                    case CHEngine::ScriptFieldType::Vec4:
                        field.Value = f["Value"].as<glm::vec4>();
                        break;
                    case CHEngine::ScriptFieldType::Color:
                        field.Value = f["Value"].as<CHEngine::Color>();
                        break;

                    case CHEngine::ScriptFieldType::Entity:
                        field.Value = f["Value"].as<uint64_t>();
                        break;
                    default:
                        break;
                    }
                }
                rhs.Fields[field.Name] = field;
            }
        }
        return true;
    }
};

inline Emitter& operator<<(Emitter& out, const CHEngine::ManagedScriptInstance& v)
{
    out << BeginMap;
    out << Key << "ClassName" << Value << v.ClassName;
    if (!v.Fields.empty())
    {
        out << Key << "Fields" << Value << BeginSeq;
        for (const auto& [name, field] : v.Fields)
        {
            out << BeginMap;
            out << Key << "Name" << Value << field.Name;
            out << Key << "Type" << Value << (int)field.Type;
            out << Key << "Value" << Value;
            std::visit([&](auto&& arg) { out << YAML::Node(arg); }, field.Value);
            out << EndMap;
        }
        out << EndSeq;
    }
    out << EndMap;
    return out;
}

} // namespace YAML

#endif // CH_SCRIPTING_SERIALIZATION_H
