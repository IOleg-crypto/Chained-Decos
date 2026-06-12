#ifndef CH_YAML_CONVERSIONS_H
#define CH_YAML_CONVERSIONS_H

#include "engine/assets/asset.h"
#include "engine/foundation/color.h"
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <variant>
#include <yaml-cpp/yaml.h>

// TODO : some day , maybe i refactor this
namespace YAML
{

template <> struct convert<glm::vec2>
{
    static Node encode(const glm::vec2& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        return node;
    }

    static bool decode(const Node& node, glm::vec2& rhs)
    {
        if (node.IsSequence() && node.size() == 2)
        {
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
        else if (node.IsMap())
        {
            rhs.x = (node["x"] ? node["x"] : node["X"]) ? (node["x"] ? node["x"] : node["X"]).as<float>() : 0.0f;
            rhs.y = (node["y"] ? node["y"] : node["Y"]) ? (node["y"] ? node["y"] : node["Y"]).as<float>() : 0.0f;
            return true;
        }
        return false;
    }
};

template <> struct convert<glm::vec3>
{
    static Node encode(const glm::vec3& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, glm::vec3& rhs)
    {
        if (node.IsSequence() && node.size() == 3)
        {
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
        else if (node.IsMap())
        {
            rhs.x = (node["x"] ? node["x"] : node["X"]) ? (node["x"] ? node["x"] : node["X"]).as<float>() : 0.0f;
            rhs.y = (node["y"] ? node["y"] : node["Y"]) ? (node["y"] ? node["y"] : node["Y"]).as<float>() : 0.0f;
            rhs.z = (node["z"] ? node["z"] : node["Z"]) ? (node["z"] ? node["z"] : node["Z"]).as<float>() : 0.0f;
            return true;
        }
        return false;
    }
};

template <> struct convert<glm::vec4>
{
    static Node encode(const glm::vec4& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.push_back(rhs.w);
        return node;
    }

    static bool decode(const Node& node, glm::vec4& rhs)
    {
        if (node.IsSequence() && node.size() == 4)
        {
            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
        return false;
    }
};

template <> struct convert<glm::quat>
{
    static Node encode(const glm::quat& rhs)
    {
        Node node;
        node.push_back(rhs.w);
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, glm::quat& rhs)
    {
        if (node.IsSequence() && node.size() == 4)
        {
            rhs.w = node[0].as<float>();
            rhs.x = node[1].as<float>();
            rhs.y = node[2].as<float>();
            rhs.z = node[3].as<float>();
            return true;
        }
        return false;
    }
};

template <> struct convert<Chained::Color>
{
    static Node encode(const Chained::Color& rhs)
    {
        Node node;
        node.push_back(static_cast<uint32_t>(rhs.r));
        node.push_back(static_cast<uint32_t>(rhs.g));
        node.push_back(static_cast<uint32_t>(rhs.b));
        node.push_back(static_cast<uint32_t>(rhs.a));
        return node;
    }

    static bool decode(const Node& node, Chained::Color& rhs)
    {
        if (node.IsSequence() && node.size() == 4)
        {
            rhs.r = static_cast<unsigned char>(node[0].as<int>());
            rhs.g = static_cast<unsigned char>(node[1].as<int>());
            rhs.b = static_cast<unsigned char>(node[2].as<int>());
            rhs.a = static_cast<unsigned char>(node[3].as<int>());
            return true;
        }
        else if (node.IsMap())
        {
            auto rNode = node["r"] ? node["r"] : node["R"];
            auto gNode = node["g"] ? node["g"] : node["G"];
            auto bNode = node["b"] ? node["b"] : node["B"];
            auto aNode = node["a"] ? node["a"] : node["A"];

            rhs.r = rNode ? static_cast<unsigned char>(rNode.as<int>()) : 255;
            rhs.g = gNode ? static_cast<unsigned char>(gNode.as<int>()) : 255;
            rhs.b = bNode ? static_cast<unsigned char>(bNode.as<int>()) : 255;
            rhs.a = aNode ? static_cast<unsigned char>(aNode.as<int>()) : 255;
            return true;
        }
        return false;
    }
};

template <> struct convert<Chained::AssetHandle>
{
    static Node encode(const Chained::AssetHandle& rhs)
    {
        Node node;
        node = (uint64_t)rhs;
        return node;
    }

    static bool decode(const Node& node, Chained::AssetHandle& rhs)
    {
        rhs = node.as<uint64_t>();
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Chained::Color& color)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << static_cast<uint32_t>(color.r) << static_cast<uint32_t>(color.g)
        << static_cast<uint32_t>(color.b) << static_cast<uint32_t>(color.a) << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& q)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << q.w << q.x << q.y << q.z << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const std::filesystem::path& p)
{
    out << p.string();
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, char* s)
{
    out << (const char*)s;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Chained::AssetHandle& handle)
{
    out << (uint64_t)handle;
    return out;
}

template <typename... Ts> inline Emitter& operator<<(Emitter& out, const std::variant<Ts...>& v)
{
    std::visit([&](auto&& arg) { out << arg; }, v);
    return out;
}

} // namespace YAML

#endif // CH_YAML_CONVERSIONS_H
