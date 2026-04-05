#ifndef CH_SERIALIZATION_UTILS_H
#define CH_SERIALIZATION_UTILS_H

#include "engine/core/uuid.h"
#include "engine/core/yaml.h"
#include "engine/core/reflection.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine::SerializationUtils
{
// --- YAML Serialization Helpers ---

template <typename T> inline void SerializeProperty(YAML::Emitter& out, const char* name, const T& value)
{
    out << YAML::Key << name << YAML::Value << value;
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
        value = node[name].as<T>();
    }
}

inline void DeserializePath(YAML::Node node, const char* name, std::string& path)
{
    if (node[name])
    {
        std::string pathValue = node[name].as<std::string>();
        if (pathValue.empty())
        {
            path = "";
            return;
        }

        // Convert to absolute path immediately on load
        path = Project::GetAbsolutePath(pathValue).generic_string();

        // Unify slashes for cross-platform portability.
#ifdef CH_PLATFORM_WINDOWS
        std::replace(path.begin(), path.end(), '\\', '/');
#endif
    }
}

inline void DeserializeHandle(YAML::Node node, const char* name, uint64_t& handle)
{
    if (node[name])
    {
        handle = node[name].as<uint64_t>();
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

// For optional components or complex types that might be missing
template <typename T> inline bool HasProperty(YAML::Node node, const char* name)
{
    return node[name].IsDefined();
}

// ========================================================================
// PropertyArchive - Declarative Serialization/Deserialization
// ========================================================================
// Allows defining component layout once for both save/load operations

class PropertyArchive
{
public:
    enum Mode
    {
        Serialize,
        Deserialize
    };

    // Serialize constructor
    PropertyArchive(YAML::Emitter& emitter)
        : m_Mode(Serialize),
          m_Out(&emitter),
          m_Node()
    {
    }

    // Deserialize constructor
    PropertyArchive(YAML::Node node)
        : m_Mode(Deserialize),
          m_Out(nullptr),
          m_Node(node)
    {
    }

    // Check if property exists (for backwards compatibility/migrations)
    bool HasProperty(const char* name)
    {
        if (m_Mode == Serialize)
        {
            return true; // Always "has" property in serialize mode (conceptually) or return false?
                         // Actually for migration checks we usually check if generic properties exist in READ mode.
        }
        return m_Node[name].IsDefined();
    }

    // Generic property handler
    template <typename T> bool operator()(const char* name, T& value)
    {
        if (m_Mode == Serialize)
        {
            SerializeProperty(*m_Out, name, value);
        }
        else
        {
            DeserializeProperty(m_Node, name, value);
        }
        return false; // No UI change tracking in serialization
    }

    // Generic field property
    template <typename T> bool Property(const char* name, T& value)
    {
        return (*this)(name, value);
    }

    // Enum property (names and count ignored for now, but signature matches reflection)
    template <typename T_Enum>
    bool Property(const char* name, T_Enum& value, const char** names, int count)
    {
        return (*this)(name, value);
    }

    // --- Property methods with metadata (ignored in serialization) ---
    template <typename T> bool Property(const char* name, T& value, const PropertyMeta& meta)
    {
        // Metadata is only used by UI, not by serialization
        return (*this)(name, value);
    }

    template <typename T_Enum>
    bool Property(const char* name, T_Enum& value, const char** names, int count, const PropertyMeta& meta)
    {
        return (*this)(name, value);
    }

    bool File(const char* name, std::string& path, const char* extensions, const PropertyMeta& meta)
    {
        // Metadata is only used by UI, not by serialization
        return File(name, path, extensions);
    }

    // File/Path property (handles relative/absolute conversion)
    bool File(const char* name, std::string& path, const char* extensions = nullptr)
    {
        if (m_Mode == Serialize)
        {
            SerializePath(*m_Out, name, path);
        }
        else
        {
            DeserializePath(m_Node, name, path);
        }
        return false;
    }

    bool Path(const char* name, std::filesystem::path& path)
    {
        if (m_Mode == Serialize)
        {
            SerializePath(*m_Out, name, path.string());
        }
        else
        {
            DeserializePath(m_Node, name, path);
        }
        return false;
    }

    bool Handle(const char* name, uint64_t& handle)
    {
        if (m_Mode == Serialize)
        {
            SerializeHandle(*m_Out, name, handle);
        }
        else
        {
            DeserializeHandle(m_Node, name, handle);
        }
        return false;
    }

    bool Handle(const char* name, UUID& handle)
    {
        uint64_t& id = (uint64_t&)handle;
        if (m_Mode == Serialize)
        {
            SerializeHandle(*m_Out, name, id);
        }
        else
        {
            DeserializeHandle(m_Node, name, id);
        }
        return false;
    }

    // --- Reflection Compatibility ---

    // Actions are ignored in serialization but we need a stub
    bool Action(const char* label, std::function<void()> func)
    {
        return false;
    }

    void Header(const char* label) {}
    void Separator() {}
    bool BeginGroup(const char* label, bool defaultOpen = true) { return true; }
    void EndGroup() {}

    // YAML doesn't track change states like UI, so we return false for now
    // (Actual change tracking is done when we SAVE the file anyway)
    bool HasChanged() const { return false; }
    void SetChanged(bool changed) {}

    ReflectionMode GetReflectionMode() const
    {
        return m_Mode == Serialize ? ReflectionMode::Serialize : ReflectionMode::Deserialize;
    }

    // Nested structure (UIStyle, TextStyle, etc.)
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

    // Array/Sequence property
    template <typename T> bool Sequence(const char* name, std::vector<T>& vec)
    {
        if (m_Mode == Serialize)
        {
            *m_Out << YAML::Key << name << YAML::Value << YAML::BeginSeq;
            for (const auto& item : vec)
            {
                *m_Out << item;
            }
            *m_Out << YAML::EndSeq;
        }
        else
        {
            if (m_Node[name] && m_Node[name].IsSequence())
            {
                vec.clear();
                for (auto item : m_Node[name])
                {
                    vec.push_back(item.template as<T>());
                }
            }
        }
        return false;
    }

    Mode GetMode() const
    {
        return m_Mode;
    }
    YAML::Emitter* GetEmitter()
    {
        return m_Out;
    }
    YAML::Node GetNode()
    {
        return m_Node;
    }

private:
    Mode m_Mode;
    YAML::Emitter* m_Out;
    YAML::Node m_Node;
};
} // namespace CHEngine::SerializationUtils

#endif // CH_SERIALIZATION_UTILS_H
