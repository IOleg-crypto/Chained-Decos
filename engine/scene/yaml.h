#ifndef CH_YAML_UTILS_H
#define CH_YAML_UTILS_H

#include <cassert>

#include "engine/scene/components.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <variant>

#include "engine/core/ch_math.h"
#include "engine/scene/project.h"
#include "yaml-cpp/yaml.h"

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

template <> struct convert<CHEngine::Color>
{
    static Node encode(const CHEngine::Color& rhs)
    {
        Node node;
        node.push_back(static_cast<uint32_t>(rhs.r));
        node.push_back(static_cast<uint32_t>(rhs.g));
        node.push_back(static_cast<uint32_t>(rhs.b));
        node.push_back(static_cast<uint32_t>(rhs.a));
        return node;
    }

    static bool decode(const Node& node, CHEngine::Color& rhs)
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

template <> struct convert<CHEngine::UIStyle>
{
    static Node encode(const CHEngine::UIStyle& rhs)
    {
        Node node;
        node.push_back(rhs.BackgroundColor);
        node.push_back(rhs.HoverColor);
        node.push_back(rhs.PressedColor);
        return node;
    }

    static bool decode(const Node& node, CHEngine::UIStyle& rhs)
    {
        if (node.IsSequence() && node.size() >= 3)
        {
            rhs.BackgroundColor = node[0].as<CHEngine::Color>();
            rhs.HoverColor = node[1].as<CHEngine::Color>();
            rhs.PressedColor = node[2].as<CHEngine::Color>();
            return true;
        }
        return false;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::Color& color)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << static_cast<uint32_t>(color.r) << static_cast<uint32_t>(color.g)
        << static_cast<uint32_t>(color.b) << static_cast<uint32_t>(color.a) << YAML::EndSeq;
    return out;
}

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::UIStyle& style)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << style.BackgroundColor << style.HoverColor << style.PressedColor << YAML::EndSeq;
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

// operator<< for glm::quat uses glm::vec4 overload

template <typename... Ts> inline Emitter& operator<<(Emitter& out, const std::variant<Ts...>& v)
{
    std::visit([&](auto&& arg) { out << arg; }, v);
    return out;
}

// ---- TextStyle ----
template <> struct convert<CHEngine::TextStyle>
{
    static Node encode(const CHEngine::TextStyle& rhs)
    {
        Node node;
        node["FontName"] = rhs.FontName;
        node["FontSize"] = rhs.FontSize;
        node["TextColor"] = rhs.TextColor;
        node["Shadow"] = rhs.Shadow;
        node["ShadowOffset"] = rhs.ShadowOffset;
        node["ShadowColor"] = rhs.ShadowColor;
        node["LetterSpacing"] = rhs.LetterSpacing;
        node["LineHeight"] = rhs.LineHeight;
        node["Horizontal"] = static_cast<int>(rhs.Horizontal);
        node["Vertical"] = static_cast<int>(rhs.Vertical);
        return node;
    }
    static bool decode(const Node& node, CHEngine::TextStyle& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }
        if (node["FontName"])
        {
            rhs.FontName = node["FontName"].as<std::string>();
        }
        if (node["FontSize"])
        {
            rhs.FontSize = node["FontSize"].as<float>();
        }
        if (node["TextColor"])
        {
            rhs.TextColor = node["TextColor"].as<CHEngine::Color>();
        }
        if (node["Shadow"])
        {
            rhs.Shadow = node["Shadow"].as<bool>();
        }
        if (node["ShadowOffset"])
        {
            rhs.ShadowOffset = node["ShadowOffset"].as<float>();
        }
        if (node["ShadowColor"])
        {
            rhs.ShadowColor = node["ShadowColor"].as<CHEngine::Color>();
        }
        if (node["LetterSpacing"])
        {
            rhs.LetterSpacing = node["LetterSpacing"].as<float>();
        }
        if (node["LineHeight"])
        {
            rhs.LineHeight = node["LineHeight"].as<float>();
        }
        if (node["Horizontal"])
        {
            rhs.Horizontal = static_cast<CHEngine::HorizontalAlignment>(node["Horizontal"].as<int>());
        }
        if (node["Vertical"])
        {
            rhs.Vertical = static_cast<CHEngine::VerticalAlignment>(node["Vertical"].as<int>());
        }
        // Support legacy names for backward compatibility if needed, but let's stick to new ones for now
        if (node["HorizontalAlignment"])
        {
            rhs.Horizontal = static_cast<CHEngine::HorizontalAlignment>(node["HorizontalAlignment"].as<int>());
        }
        if (node["VerticalAlignment"])
        {
            rhs.Vertical = static_cast<CHEngine::VerticalAlignment>(node["VerticalAlignment"].as<int>());
        }
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::TextStyle& s)
{
    out << YAML::BeginMap << YAML::Key << "FontName" << YAML::Value << s.FontName << YAML::Key << "FontSize"
        << YAML::Value << s.FontSize << YAML::Key << "TextColor" << YAML::Value << s.TextColor << YAML::Key << "Shadow"
        << YAML::Value << s.Shadow << YAML::Key << "ShadowOffset" << YAML::Value << s.ShadowOffset << YAML::Key
        << "ShadowColor" << YAML::Value << s.ShadowColor << YAML::Key << "LetterSpacing" << YAML::Value
        << s.LetterSpacing << YAML::Key << "LineHeight" << YAML::Value << s.LineHeight << YAML::Key
        << "Horizontal" << YAML::Value << static_cast<int>(s.Horizontal) << YAML::Key
        << "Vertical" << YAML::Value << static_cast<int>(s.Vertical) << YAML::EndMap;
    return out;
}

// ---- RectTransform ----
template <> struct convert<CHEngine::RectTransform>
{
    static Node encode(const CHEngine::RectTransform& rhs)
    {
        Node node;
        node["AnchorMin"] = rhs.AnchorMin;
        node["AnchorMax"] = rhs.AnchorMax;
        node["OffsetMin"] = rhs.OffsetMin;
        node["OffsetMax"] = rhs.OffsetMax;
        node["Pivot"] = rhs.Pivot;
        node["Rotation"] = rhs.Rotation;
        node["Scale"] = rhs.Scale;
        return node;
    }
    static bool decode(const Node& node, CHEngine::RectTransform& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }
        if (node["AnchorMin"])
        {
            rhs.AnchorMin = node["AnchorMin"].as<glm::vec2>();
        }
        if (node["AnchorMax"])
        {
            rhs.AnchorMax = node["AnchorMax"].as<glm::vec2>();
        }
        if (node["OffsetMin"])
        {
            rhs.OffsetMin = node["OffsetMin"].as<glm::vec2>();
        }
        if (node["OffsetMax"])
        {
            rhs.OffsetMax = node["OffsetMax"].as<glm::vec2>();
        }
        if (node["Pivot"])
        {
            rhs.Pivot = node["Pivot"].as<glm::vec2>();
        }
        if (node["Rotation"])
        {
            rhs.Rotation = node["Rotation"].as<float>();
        }
        if (node["Scale"])
        {
            rhs.Scale = node["Scale"].as<glm::vec2>();
        }
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::RectTransform& t)
{
    out << YAML::BeginMap << YAML::Key << "AnchorMin" << YAML::Value << t.AnchorMin << YAML::Key << "AnchorMax"
        << YAML::Value << t.AnchorMax << YAML::Key << "OffsetMin" << YAML::Value << t.OffsetMin << YAML::Key
        << "OffsetMax" << YAML::Value << t.OffsetMax << YAML::Key << "Pivot" << YAML::Value << t.Pivot << YAML::Key
        << "Rotation" << YAML::Value << t.Rotation << YAML::Key << "Scale" << YAML::Value << t.Scale << YAML::EndMap;
    return out;
}

// ---- MaterialInstance ----
template <> struct convert<CHEngine::MaterialInstance>
{
    static Node encode(const CHEngine::MaterialInstance& rhs)
    {
        Node node;
        node["AlbedoColor"] = rhs.AlbedoColor;
        node["AlbedoPath"] = CHEngine::Project::GetRelativePath(rhs.AlbedoPath);
        node["OverrideAlbedo"] = rhs.OverrideAlbedo;
        node["NormalMapPath"] = CHEngine::Project::GetRelativePath(rhs.NormalMapPath);
        node["OverrideNormal"] = rhs.OverrideNormal;
        node["MetallicRoughnessPath"] = CHEngine::Project::GetRelativePath(rhs.MetallicRoughnessPath);
        node["OverrideMetallicRoughness"] = rhs.OverrideMetallicRoughness;
        node["OcclusionMapPath"] = CHEngine::Project::GetRelativePath(rhs.OcclusionMapPath);
        node["OverrideOcclusion"] = rhs.OverrideOcclusion;
        node["EmissivePath"] = CHEngine::Project::GetRelativePath(rhs.EmissivePath);
        node["EmissiveColor"] = rhs.EmissiveColor;
        node["EmissiveIntensity"] = rhs.EmissiveIntensity;
        node["OverrideEmissive"] = rhs.OverrideEmissive;
        node["ShaderPath"] = CHEngine::Project::GetRelativePath(rhs.ShaderPath);
        node["OverrideShader"] = rhs.OverrideShader;
        node["Metalness"] = rhs.Metalness;
        node["Roughness"] = rhs.Roughness;
        node["DoubleSided"] = rhs.DoubleSided;
        node["Transparent"] = rhs.Transparent;
        node["Alpha"] = rhs.Alpha;
        return node;
    }
    static bool decode(const Node& node, CHEngine::MaterialInstance& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }
        if (node["AlbedoColor"])
        {
            rhs.AlbedoColor = node["AlbedoColor"].as<CHEngine::Color>();
        }
        if (node["AlbedoPath"])
        {
            rhs.AlbedoPath = node["AlbedoPath"].as<std::string>();
        }
        if (node["OverrideAlbedo"])
        {
            rhs.OverrideAlbedo = node["OverrideAlbedo"].as<bool>();
        }
        if (node["NormalMapPath"])
        {
            rhs.NormalMapPath = node["NormalMapPath"].as<std::string>();
        }
        if (node["OverrideNormal"])
        {
            rhs.OverrideNormal = node["OverrideNormal"].as<bool>();
        }
        if (node["MetallicRoughnessPath"])
        {
            rhs.MetallicRoughnessPath = node["MetallicRoughnessPath"].as<std::string>();
        }
        if (node["OverrideMetallicRoughness"])
        {
            rhs.OverrideMetallicRoughness = node["OverrideMetallicRoughness"].as<bool>();
        }
        if (node["OcclusionMapPath"])
        {
            rhs.OcclusionMapPath = node["OcclusionMapPath"].as<std::string>();
        }
        if (node["OverrideOcclusion"])
        {
            rhs.OverrideOcclusion = node["OverrideOcclusion"].as<bool>();
        }
        if (node["EmissivePath"])
        {
            rhs.EmissivePath = node["EmissivePath"].as<std::string>();
        }
        if (node["EmissiveColor"])
        {
            rhs.EmissiveColor = node["EmissiveColor"].as<CHEngine::Color>();
        }
        if (node["EmissiveIntensity"])
        {
            rhs.EmissiveIntensity = node["EmissiveIntensity"].as<float>();
        }
        if (node["OverrideEmissive"])
        {
            rhs.OverrideEmissive = node["OverrideEmissive"].as<bool>();
        }
        if (node["ShaderPath"])
        {
            rhs.ShaderPath = node["ShaderPath"].as<std::string>();
        }
        if (node["OverrideShader"])
        {
            rhs.OverrideShader = node["OverrideShader"].as<bool>();
        }
        if (node["Metalness"])
        {
            rhs.Metalness = node["Metalness"].as<float>();
        }
        if (node["Roughness"])
        {
            rhs.Roughness = node["Roughness"].as<float>();
        }
        if (node["DoubleSided"])
        {
            rhs.DoubleSided = node["DoubleSided"].as<bool>();
        }
        if (node["Transparent"])
        {
            rhs.Transparent = node["Transparent"].as<bool>();
        }
        if (node["Alpha"])
        {
            rhs.Alpha = node["Alpha"].as<float>();
        }
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::MaterialInstance& m)
{
    out << YAML::BeginMap << YAML::Key << "AlbedoColor" << YAML::Value << m.AlbedoColor << YAML::Key << "AlbedoPath"
        << YAML::Value << CHEngine::Project::GetRelativePath(m.AlbedoPath) << YAML::Key << "OverrideAlbedo"
        << YAML::Value << m.OverrideAlbedo << YAML::Key << "NormalMapPath" << YAML::Value
        << CHEngine::Project::GetRelativePath(m.NormalMapPath) << YAML::Key << "OverrideNormal" << YAML::Value
        << m.OverrideNormal << YAML::Key << "MetallicRoughnessPath" << YAML::Value
        << CHEngine::Project::GetRelativePath(m.MetallicRoughnessPath) << YAML::Key << "OverrideMetallicRoughness"
        << YAML::Value << m.OverrideMetallicRoughness << YAML::Key << "OcclusionMapPath" << YAML::Value
        << CHEngine::Project::GetRelativePath(m.OcclusionMapPath) << YAML::Key << "OverrideOcclusion" << YAML::Value
        << m.OverrideOcclusion << YAML::Key << "EmissivePath" << YAML::Value
        << CHEngine::Project::GetRelativePath(m.EmissivePath) << YAML::Key << "EmissiveColor" << YAML::Value
        << m.EmissiveColor << YAML::Key << "EmissiveIntensity" << YAML::Value << m.EmissiveIntensity << YAML::Key
        << "OverrideEmissive" << YAML::Value << m.OverrideEmissive << YAML::Key << "ShaderPath" << YAML::Value
        << CHEngine::Project::GetRelativePath(m.ShaderPath) << YAML::Key << "OverrideShader" << YAML::Value
        << m.OverrideShader << YAML::Key << "Metalness" << YAML::Value << m.Metalness << YAML::Key << "Roughness"
        << YAML::Value << m.Roughness << YAML::Key << "DoubleSided" << YAML::Value << m.DoubleSided << YAML::Key
        << "Transparent" << YAML::Value << m.Transparent << YAML::Key << "Alpha" << YAML::Value << m.Alpha
        << YAML::EndMap;
    return out;
}

// ---- MaterialSlot ----
template <> struct convert<CHEngine::MaterialSlot>
{
    static Node encode(const CHEngine::MaterialSlot& rhs)
    {
        Node node;
        node["Name"] = rhs.Name;
        node["Index"] = rhs.Index;
        node["Target"] = static_cast<int>(rhs.Target);
        node["Material"] = rhs.Material;
        return node;
    }
    static bool decode(const Node& node, CHEngine::MaterialSlot& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }
        if (node["Name"])
        {
            rhs.Name = node["Name"].as<std::string>();
        }
        if (node["Index"])
        {
            rhs.Index = node["Index"].as<int>();
        }
        if (node["Target"])
        {
            rhs.Target = static_cast<CHEngine::MaterialSlotTarget>(node["Target"].as<int>());
        }
        if (node["Material"])
        {
            rhs.Material = node["Material"].as<CHEngine::MaterialInstance>();
        }
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::MaterialSlot& s)
{
    out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << s.Name << YAML::Key << "Index" << YAML::Value
        << s.Index << YAML::Key << "Target" << YAML::Value << static_cast<int>(s.Target) << YAML::Key << "Material"
        << YAML::Value << s.Material << YAML::EndMap;
    return out;
}

// ---- ShaderUniform ----
template <> struct convert<CHEngine::ShaderUniform>
{
    static Node encode(const CHEngine::ShaderUniform& rhs)
    {
        Node node;
        node["Name"] = rhs.Name;
        node["Type"] = rhs.Type;
        node["Value"] = glm::vec4(rhs.Value[0], rhs.Value[1], rhs.Value[2], rhs.Value[3]);
        return node;
    }
    static bool decode(const Node& node, CHEngine::ShaderUniform& rhs)
    {
        if (!node.IsMap())
        {
            return false;
        }
        if (node["Name"])
        {
            rhs.Name = node["Name"].as<std::string>();
        }
        if (node["Type"])
        {
            rhs.Type = node["Type"].as<int>();
        }
        if (node["Value"])
        {
            glm::vec4 v = node["Value"].as<glm::vec4>();
            rhs.Value[0] = v.x;
            rhs.Value[1] = v.y;
            rhs.Value[2] = v.z;
            rhs.Value[3] = v.w;
        }
        return true;
    }
};

inline YAML::Emitter& operator<<(YAML::Emitter& out, const CHEngine::ShaderUniform& u)
{
    out << YAML::BeginMap << YAML::Key << "Name" << YAML::Value << u.Name << YAML::Key << "Type" << YAML::Value
        << u.Type << YAML::Key << "Value" << YAML::Value << glm::vec4(u.Value[0], u.Value[1], u.Value[2], u.Value[3])
        << YAML::EndMap;
    return out;
}

} // namespace YAML

#endif // CH_YAML_UTILS_H
