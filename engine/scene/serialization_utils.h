#ifndef CH_SERIALIZATION_UTILS_H
#define CH_SERIALIZATION_UTILS_H

#include "engine/core/uuid.h"
#include "engine/scene/yaml.h"
#include "engine/core/reflection.h"
#include "engine/scene/project.h"
#include "engine/scene/scripting_serialization.h"
#include <filesystem>

namespace CHEngine::SerializationUtils
{
// --- YAML Serialization Helpers ---

template <typename T> inline void SerializeProperty(YAML::Emitter& out, const char* name, const T& value)
{
    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t> || std::is_same_v<T, unsigned char> || std::is_same_v<T, char>)
    {
        out << YAML::Key << name << YAML::Value << static_cast<int>(value);
    }
    else
    {
        out << YAML::Key << name << YAML::Value << value;
    }
}

inline void SerializePath(YAML::Emitter& out, const char* name, const std::string& path)
{
    if (path.empty())
    {
        out << YAML::Key << name << YAML::Value << "";
        return;
    }

    std::string relativePath = Project::GetRelativePath(path);
    out << YAML::Key << name << YAML::Value << relativePath;
}

inline void SerializeHandle(YAML::Emitter& out, const char* name, uint64_t handle)
{
    out << YAML::Key << name << YAML::Value << handle;
}

// --- YAML Deserialization Helpers ---

template <typename T> inline void DeserializeProperty(YAML::Node node, const char* name, T& value)
{
    if (node[name])
    {
        if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t> || std::is_same_v<T, unsigned char> || std::is_same_v<T, char>)
        {
            value = static_cast<T>(node[name].as<int>(static_cast<int>(value)));
        }
        else
        {
            value = node[name].as<T>(value);
        }
    }
}

inline void DeserializePath(YAML::Node node, const char* name, std::string& path)
{
    if (node[name])
    {
        std::string pathValue = node[name].as<std::string>(path);
        if (pathValue.empty())
        {
            path = "";
            return;
        }
        path = pathValue;
#if CH_PLATFORM_WINDOWS
        std::replace(path.begin(), path.end(), '\\', '/');
#endif
    }
}

inline void DeserializeHandle(YAML::Node node, const char* name, uint64_t& handle)
{
    if (node[name])
    {
        handle = node[name].as<uint64_t>(handle);
    }
}

inline void DeserializePath(YAML::Node node, const char* name, std::filesystem::path& path)
{
    std::string pathStr;
    DeserializePath(node, name, pathStr);
    if (!pathStr.empty())
    {
        path = pathStr;
    }
}

// ========================================================================
// PropertyArchive - Declarative Serialization/Deserialization
// ========================================================================

class PropertyArchive : public IPropertyArchive
{
public:
    enum Mode
    {
        Serialize,
        Deserialize
    };

    PropertyArchive(YAML::Emitter& emitter)
        : m_Mode(Serialize), m_Out(&emitter)
    {
    }

    PropertyArchive(YAML::Node node)
        : m_Mode(Deserialize), m_Out(nullptr), m_Node(node)
    {
    }

    virtual ReflectionMode GetReflectionMode() const override
    {
        return m_Mode == Serialize ? ReflectionMode::Serialize : ReflectionMode::Deserialize;
    }

    virtual bool Property(const char* name, int& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, float& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, bool& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, std::string& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, glm::vec2& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, glm::vec3& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, glm::vec4& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, Color& value, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    virtual bool Property(const char* name, uint64_t& value, const PropertyMeta& meta = {}) override { return Handle(name, value, meta); }
    virtual bool Enum(const char* name, int& value, const char** names, int count, const PropertyMeta& meta = {}) override { return PropertyInternal(name, value); }
    
    virtual bool Handle(const char* name, uint64_t& handle, const PropertyMeta& meta = {}) override 
    {
        if (m_Mode == Serialize) SerializeHandle(*m_Out, name, handle);
        else DeserializeHandle(m_Node, name, handle);
        return false;
    }
    virtual bool File(const char* name, std::string& path, const char* extensions = nullptr, const PropertyMeta& meta = {}) override
    {
        if (m_Mode == Serialize) SerializePath(*m_Out, name, path);
        else DeserializePath(m_Node, name, path);
        return false;
    }
    virtual bool Action(const char* label, std::function<void()> func) override { return false; }
    virtual void Header(const char* label) override {}
    virtual void Separator() override {}
    virtual bool BeginGroup(const char* label, bool defaultOpen = true) override { return true; }
    virtual void EndGroup() override {}
    virtual bool HasChanged() const override { return false; }
    virtual void SetChanged(bool changed) override {}

    virtual void BeginSequence(const char* name, size_t& size) override
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::Key << name << YAML::Value << YAML::BeginSeq;
        }
        else
        {
            if (m_Node[name] && m_Node[name].IsSequence())
            {
                size = m_Node[name].size();
                // To allow iterating over elements during deserialization, we might need state.
                // However, Properties::Sequence current implementation uses m_Node[name][i] via recursive calls.
                // For simplicity, we just stay with this.
            }
        }
    }

    virtual void EndSequence() override
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::EndSeq;
        }
    }

    virtual bool Nested(const char* name, std::function<void(IPropertyArchive&)> callback) override
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::Key << name << YAML::Value << YAML::BeginMap;
            PropertyArchive nestedArchive(*m_Out);
            callback(static_cast<IPropertyArchive&>(nestedArchive));
            *m_Out << YAML::EndMap;
        }
        else
        {
            if (m_Node[name])
            {
                PropertyArchive nestedArchive(m_Node[name]);
                callback(static_cast<IPropertyArchive&>(nestedArchive));
            }
        }
        return false;
    }

    // Template methods for backward compatibility
    template <typename T> bool Property(const char* name, T& value) { return PropertyInternal(name, value); }
    template <typename T_Enum> bool Property(const char* name, T_Enum& value, const char** names, int count) { return PropertyInternal(name, (int&)value); }
    template <typename T> bool Property(const char* name, T& value, const PropertyMeta& meta) { return PropertyInternal(name, value); }
    template <typename T_Enum> bool Property(const char* name, T_Enum& value, const char** names, int count, const PropertyMeta& meta) { return PropertyInternal(name, (int&)value); }

    bool Path(const char* name, std::filesystem::path& path)
    {
        if (m_Mode == Serialize) SerializePath(*m_Out, name, path.string());
        else DeserializePath(m_Node, name, path);
        return false;
    }

    template <typename T>
    bool Nested(const char* name, T& value)
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::Key << name << YAML::Value << YAML::BeginMap;
            PropertyArchive nestedArchive(*m_Out);
            Properties<PropertyArchive> p(nestedArchive);
            value.Reflect(p);
            *m_Out << YAML::EndMap;
        }
        else
        {
            if (m_Node[name])
            {
                PropertyArchive nestedArchive(m_Node[name]);
                Properties<PropertyArchive> p(nestedArchive);
                value.Reflect(p);
            }
        }
        return false;
    }

    template <typename T> bool Sequence(const char* name, std::vector<T>& vec, bool allowAddRemove = true)
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::Key << name << YAML::Value << YAML::BeginSeq;
            for (const auto& item : vec) *m_Out << item;
            *m_Out << YAML::EndSeq;
        }
        else
        {
            if (m_Node[name] && m_Node[name].IsSequence())
            {
                vec.clear();
                for (auto item : m_Node[name]) vec.push_back(item.as<T>(T{}));
            }
        }
        return false;
    }

private:
    template<typename T> bool PropertyInternal(const char* name, T& value)
    {
        if (m_Mode == Serialize) SerializeProperty(*m_Out, name, value);
        else DeserializeProperty(m_Node, name, value);
        return false;
    }

    Mode m_Mode;
    YAML::Emitter* m_Out = nullptr;
    YAML::Node m_Node;
};
} // namespace CHEngine::SerializationUtils

#endif // CH_SERIALIZATION_UTILS_H
