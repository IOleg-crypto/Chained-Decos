#ifndef CH_YAML_UTILS_H
#define CH_YAML_UTILS_H

#include "engine/scene/components.h"
#include "engine/foundation/color.h"
#include "engine/runtime/application.h"
#include "yaml-cpp/yaml.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <variant>
#include <filesystem>

namespace YAML
{
    // --- ВНУТРІШНІ ХЕЛПЕРИ ДЛЯ СКОРОЧЕННЯ БОЙЛЕРПЛЕЙТУ ---
    template<typename T>
    inline void DecodeField(const Node& node, const std::string& key, T& target)
    {
        if (node[key]) target = node[key].as<T>();
    }

    template<typename T, typename EnumT>
    inline void DecodeEnum(const Node& node, const std::string& key, EnumT& target)
    {
        if (node[key]) target = static_cast<EnumT>(node[key].as<T>());
    }

    // --- GLM GLM::VEC2 ---
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
            if (node.IsMap())
            {
                rhs.x = node["x"] ? node["x"].as<float>() : (node["X"] ? node["X"].as<float>() : 0.0f);
                rhs.y = node["y"] ? node["y"].as<float>() : (node["Y"] ? node["Y"].as<float>() : 0.0f);
                return true;
            }
            return false;
        }
    };

    // --- GLM GLM::VEC3 ---
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
            if (node.IsMap())
            {
                rhs.x = node["x"] ? node["x"].as<float>() : (node["X"] ? node["X"].as<float>() : 0.0f);
                rhs.y = node["y"] ? node["y"].as<float>() : (node["Y"] ? node["Y"].as<float>() : 0.0f);
                rhs.z = node["z"] ? node["z"].as<float>() : (node["Z"] ? node["Z"].as<float>() : 0.0f);
                return true;
            }
            return false;
        }
    };

    // --- GLM GLM::VEC4 ---
    template <> struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& rhs) {
            Node node;
            node.push_back(rhs.x); node.push_back(rhs.y); node.push_back(rhs.z); node.push_back(rhs.w);
            return node;
        }
        static bool decode(const Node& node, glm::vec4& rhs) {
            if (!node.IsSequence() || node.size() != 4) return false;
            rhs.x = node[0].as<float>(); rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>(); rhs.w = node[3].as<float>();
            return true;
        }
    };

    // --- GLM GLM::QUAT ---
    template <> struct convert<glm::quat>
    {
        static Node encode(const glm::quat& rhs) {
            Node node;
            node.push_back(rhs.w); node.push_back(rhs.x); node.push_back(rhs.y); node.push_back(rhs.z);
            return node;
        }
        static bool decode(const Node& node, glm::quat& rhs) {
            if (!node.IsSequence() || node.size() != 4) return false;
            rhs.w = node[0].as<float>(); rhs.x = node[1].as<float>();
            rhs.y = node[2].as<float>(); rhs.z = node[3].as<float>();
            return true;
        }
    };

    // --- Chained::COLOR ---
    template <> struct convert<Chained::Color>
    {
        static Node encode(const Chained::Color& rhs) {
            Node node;
            node.push_back(static_cast<uint32_t>(rhs.r)); node.push_back(static_cast<uint32_t>(rhs.g));
            node.push_back(static_cast<uint32_t>(rhs.b)); node.push_back(static_cast<uint32_t>(rhs.a));
            return node;
        }
        static bool decode(const Node& node, Chained::Color& rhs) {
            if (node.IsSequence() && node.size() == 4) {
                rhs.r = static_cast<unsigned char>(node[0].as<int>());
                rhs.g = static_cast<unsigned char>(node[1].as<int>());
                rhs.b = static_cast<unsigned char>(node[2].as<int>());
                rhs.a = static_cast<unsigned char>(node[3].as<int>());
                return true;
            }
            if (node.IsMap()) {
                // Лямбда для безпечного регістронезалежного читання каналів
                auto getChannel = [&](const char* k1, const char* k2) {
                    auto n = node[k1] ? node[k1] : node[k2];
                    return n ? static_cast<unsigned char>(n.as<int>()) : 255;
                };
                rhs.r = getChannel("r", "R");
                rhs.g = getChannel("g", "G");
                rhs.b = getChannel("b", "B");
                rhs.a = getChannel("a", "A");
                return true;
            }
            return false;
        }
    };

    // --- Chained::UISTYLE ---
    template <> struct convert<Chained::UIStyle>
    {
        static Node encode(const Chained::UIStyle& rhs) {
            Node node;
            node.push_back(rhs.BackgroundColor); node.push_back(rhs.HoverColor); node.push_back(rhs.PressedColor);
            return node;
        }
        static bool decode(const Node& node, Chained::UIStyle& rhs) {
            if (!node.IsSequence() || node.size() < 3) return false;
            rhs.BackgroundColor = node[0].as<Chained::Color>();
            rhs.HoverColor = node[1].as<Chained::Color>();
            rhs.PressedColor = node[2].as<Chained::Color>();
            return true;
        }
    };

    // --- TEXTSTYLE ---
    template <> struct convert<Chained::TextStyle>
    {
        static Node encode(const Chained::TextStyle& rhs)
        {
            Node node;
            node["FontName"]      = rhs.FontName;
            node["FontSize"]      = rhs.FontSize;
            node["TextColor"]     = rhs.TextColor;
            node["Shadow"]        = rhs.Shadow;
            node["ShadowOffset"]  = rhs.ShadowOffset;
            node["ShadowColor"]   = rhs.ShadowColor;
            node["LetterSpacing"] = rhs.LetterSpacing;
            node["LineHeight"]    = rhs.LineHeight;
            node["Horizontal"]    = static_cast<int>(rhs.Horizontal);
            node["Vertical"]      = static_cast<int>(rhs.Vertical);
            return node;
        }
        static bool decode(const Node& node, Chained::TextStyle& rhs)
        {
            if (!node.IsMap()) return false;

            DecodeField(node, "FontName", rhs.FontName);
            DecodeField(node, "FontSize", rhs.FontSize);
            DecodeField(node, "TextColor", rhs.TextColor);
            DecodeField(node, "Shadow", rhs.Shadow);
            DecodeField(node, "ShadowOffset", rhs.ShadowOffset);
            DecodeField(node, "ShadowColor", rhs.ShadowColor);
            DecodeField(node, "LetterSpacing", rhs.LetterSpacing);
            DecodeField(node, "LineHeight", rhs.LineHeight);

            // Підтримка старих та нових назв енумів
            DecodeEnum<int>(node, "Horizontal", rhs.Horizontal);
            DecodeEnum<int>(node, "HorizontalAlignment", rhs.Horizontal);
            DecodeEnum<int>(node, "Vertical", rhs.Vertical);
            DecodeEnum<int>(node, "VerticalAlignment", rhs.Vertical);

            return true;
        }
    };

    // --- RECTRANSFORM ---
    template <> struct convert<Chained::RectTransform>
    {
        static Node encode(const Chained::RectTransform& rhs)
        {
            Node node;
            node["AnchorMin"] = rhs.AnchorMin;
            node["AnchorMax"] = rhs.AnchorMax;
            node["OffsetMin"] = rhs.OffsetMin;
            node["OffsetMax"] = rhs.OffsetMax;
            node["Pivot"]     = rhs.Pivot;
            node["Rotation"]  = rhs.Rotation;
            node["Scale"]     = rhs.Scale;
            return node;
        }
        static bool decode(const Node& node, Chained::RectTransform& rhs)
        {
            if (!node.IsMap()) return false;

            DecodeField(node, "AnchorMin", rhs.AnchorMin);
            DecodeField(node, "AnchorMax", rhs.AnchorMax);
            DecodeField(node, "OffsetMin", rhs.OffsetMin);
            DecodeField(node, "OffsetMax", rhs.OffsetMax);
            DecodeField(node, "Pivot",     rhs.Pivot);
            DecodeField(node, "Rotation",  rhs.Rotation);
            DecodeField(node, "Scale",     rhs.Scale);

            return true;
        }
    };

    // --- ASSETHANDLE ---
    template <> struct convert<Chained::AssetHandle>
    {
        static Node encode(const Chained::AssetHandle& rhs) { return Node(static_cast<uint64_t>(rhs)); }
        static bool decode(const Node& node, Chained::AssetHandle& rhs) { rhs = node.as<uint64_t>(); return true; }
    };

    // --- SHADERUNIFORM ---
    template <> struct convert<Chained::ShaderUniform>
    {
        static Node encode(const Chained::ShaderUniform& rhs)
        {
            Node node;
            node["Name"]  = rhs.Name;
            node["Type"]  = rhs.Type;
            node["Value"] = glm::vec4(rhs.Value[0], rhs.Value[1], rhs.Value[2], rhs.Value[3]);
            return node;
        }
        static bool decode(const Node& node, Chained::ShaderUniform& rhs)
        {
            if (!node.IsMap()) return false;

            DecodeField(node, "Name", rhs.Name);
            DecodeField(node, "Type", rhs.Type);

            if (node["Value"]) {
                glm::vec4 v = node["Value"].as<glm::vec4>();
                rhs.Value[0] = v.x; rhs.Value[1] = v.y; rhs.Value[2] = v.z; rhs.Value[3] = v.w;
            }
            return true;
        }
    };

    // --- STD::VARIANT ---
    template <typename... Ts> struct convert<std::variant<Ts...>>
    {
        static Node encode(const std::variant<Ts...>& rhs) {
            return std::visit([](auto&& arg) { return Node(arg); }, rhs);
        }
        static bool decode(const Node& node, std::variant<Ts...>& rhs) { return false; }
    };

    // ==========================================
    // EMITTER OPERATORS (Вирівняні для читабельності)
    // ==========================================

    inline Emitter& operator<<(Emitter& out, const Chained::Color& c) {
        return out << Flow << BeginSeq << static_cast<uint32_t>(c.r) << static_cast<uint32_t>(c.g)
                   << static_cast<uint32_t>(c.b) << static_cast<uint32_t>(c.a) << EndSeq;
    }

    inline Emitter& operator<<(Emitter& out, const Chained::UIStyle& s) {
        return out << Flow << BeginSeq << s.BackgroundColor << s.HoverColor << s.PressedColor << EndSeq;
    }

    inline Emitter& operator<<(Emitter& out, const glm::vec2& v) { return out << Flow << BeginSeq << v.x << v.y << EndSeq; }
    inline Emitter& operator<<(Emitter& out, const glm::vec3& v) { return out << Flow << BeginSeq << v.x << v.y << v.z << EndSeq; }
    inline Emitter& operator<<(Emitter& out, const glm::vec4& v) { return out << Flow << BeginSeq << v.x << v.y << v.z << v.w << EndSeq; }
    inline Emitter& operator<<(Emitter& out, const glm::quat& q) { return out << Flow << BeginSeq << q.w << q.x << q.y << q.z << EndSeq; }
    inline Emitter& operator<<(Emitter& out, const std::filesystem::path& p) { return out << p.string(); }

    template <typename... Ts>
    inline Emitter& operator<<(Emitter& out, const std::variant<Ts...>& v) {
        std::visit([&](auto&& arg) { out << arg; }, v); return out;
    }

    inline Emitter& operator<<(Emitter& out, const Chained::TextStyle& s)
    {
        return out << BeginMap
            << Key << "FontName"      << Value << s.FontName
            << Key << "FontSize"      << Value << s.FontSize
            << Key << "TextColor"     << Value << s.TextColor
            << Key << "Shadow"        << Value << s.Shadow
            << Key << "ShadowOffset"  << Value << s.ShadowOffset
            << Key << "ShadowColor"   << Value << s.ShadowColor
            << Key << "LetterSpacing" << Value << s.LetterSpacing
            << Key << "LineHeight"    << Value << s.LineHeight
            << Key << "Horizontal"    << Value << static_cast<int>(s.Horizontal)
            << Key << "Vertical"      << Value << static_cast<int>(s.Vertical)
            << EndMap;
    }

    inline Emitter& operator<<(Emitter& out, const Chained::RectTransform& t)
    {
        return out << BeginMap
            << Key << "AnchorMin" << Value << t.AnchorMin
            << Key << "AnchorMax" << Value << t.AnchorMax
            << Key << "OffsetMin" << Value << t.OffsetMin
            << Key << "OffsetMax" << Value << t.OffsetMax
            << Key << "Pivot"     << Value << t.Pivot
            << Key << "Rotation"  << Value << t.Rotation
            << Key << "Scale"     << Value << t.Scale
            << EndMap;
    }

    inline Emitter& operator<<(Emitter& out, const Chained::AssetHandle& handle) { return out << static_cast<uint64_t>(handle); }

    inline Emitter& operator<<(Emitter& out, const Chained::ShaderUniform& u)
    {
        return out << BeginMap
            << Key << "Name"  << Value << u.Name
            << Key << "Type"  << Value << u.Type
            << Key << "Value" << Value << glm::vec4(u.Value[0], u.Value[1], u.Value[2], u.Value[3])
            << EndMap;
    }
}

#endif // CH_YAML_UTILS_H