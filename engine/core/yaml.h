#ifndef CH_YAML_UTILS_H
#define CH_YAML_UTILS_H

#include <cassert>

#include <variant>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "yaml-cpp/yaml.h"
#include "engine/core/ch_math.h"

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
            rhs.x = node["x"] ? node["x"].as<float>() : 0.0f;
            rhs.y = node["y"] ? node["y"].as<float>() : 0.0f;
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
            rhs.x = node["x"] ? node["x"].as<float>() : 0.0f;
            rhs.y = node["y"] ? node["y"].as<float>() : 0.0f;
            rhs.z = node["z"] ? node["z"].as<float>() : 0.0f;
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

// glm::quat is a typedef of glm::vec4 in Raylib, so it uses the glm::vec4 specialization.

template <> struct convert<CHEngine::Color>
{
    static Node encode(const CHEngine::Color& rhs)
    {
        Node node;
        node.push_back(rhs.r);
        node.push_back(rhs.g);
        node.push_back(rhs.b);
        node.push_back(rhs.a);
        return node;
    }

    static bool decode(const Node& node, CHEngine::Color& rhs)
    {
        if (node.IsSequence() && node.size() == 4)
        {
            rhs.r = node[0].as<unsigned char>();
            rhs.g = node[1].as<unsigned char>();
            rhs.b = node[2].as<unsigned char>();
            rhs.a = node[3].as<unsigned char>();
            return true;
        }
        else if (node.IsMap())
        {
            rhs.r = node["r"] ? node["r"].as<unsigned char>() : 255;
            rhs.g = node["g"] ? node["g"].as<unsigned char>() : 255;
            rhs.b = node["b"] ? node["b"].as<unsigned char>() : 255;
            rhs.a = node["a"] ? node["a"].as<unsigned char>() : 255;
            return true;
        }
        return false;
    }
};


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

// operator<< for glm::quat uses glm::vec4 overload

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::Color& c)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << (int)c.r << (int)c.g << (int)c.b << (int)c.a << YAML::EndSeq;
    return out;
}

template<typename... Ts>
inline Emitter& operator<<(Emitter& out, const std::variant<Ts...>& v)
{
    std::visit([&](auto&& arg) { out << arg; }, v);
    return out;
}

} // namespace YAML

#endif // CH_YAML_UTILS_H
