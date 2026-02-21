#ifndef CH_YAML_UTILS_H
#define CH_YAML_UTILS_H

#include <raylib.h>

#include "yaml-cpp/yaml.h"

namespace YAML
{

template <> struct convert<Vector2>
{
    static Node encode(const Vector2& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        return node;
    }

    static bool decode(const Node& node, Vector2& rhs)
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

template <> struct convert<Vector3>
{
    static Node encode(const Vector3& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node& node, Vector3& rhs)
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

template <> struct convert<Vector4>
{
    static Node encode(const Vector4& rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        node.push_back(rhs.w);
        return node;
    }

    static bool decode(const Node& node, Vector4& rhs)
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

// Quaternion is a typedef of Vector4 in Raylib, so it uses the Vector4 specialization.

template <> struct convert<Color>
{
    static Node encode(const Color& rhs)
    {
        Node node;
        node.push_back(rhs.r);
        node.push_back(rhs.g);
        node.push_back(rhs.b);
        node.push_back(rhs.a);
        return node;
    }

    static bool decode(const Node& node, Color& rhs)
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
} // namespace YAML

namespace CHEngine
{

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vector2& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vector3& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vector4& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

// operator<< for Quaternion uses Vector4 overload

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Color& c)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << (int)c.r << (int)c.g << (int)c.b << (int)c.a << YAML::EndSeq;
    return out;
}
} // namespace CHEngine

#endif // CH_YAML_UTILS_H
