#ifndef CH_SERIALIZATION_UTILS_H
#define CH_SERIALIZATION_UTILS_H

#include "engine/core/yaml.h"
#include "engine/scene/project.h"
#include <filesystem>

namespace CHEngine::SerializationUtils
{
    // --- YAML Serialization Helpers ---
    
    template<typename T>
    inline void SerializeProperty(YAML::Emitter& out, const char* name, const T& value)
    {
        out << YAML::Key << name << YAML::Value << value;
    }

    inline void SerializePath(YAML::Emitter& out, const char* name, const std::string& path)
    {
        if (path.empty()) {
            out << YAML::Key << name << YAML::Value << "";
            return;
        }

        std::string relativePath = Project::GetRelativePath(path);
        out << YAML::Key << name << YAML::Value << relativePath;
    }

    // --- YAML Deserialization Helpers ---

    template<typename T>
    inline void DeserializeProperty(YAML::Node node, const char* name, T& value)
    {
        if (node[name])
            value = node[name].as<T>();
    }

    inline void DeserializePath(YAML::Node node, const char* name, std::string& path)
    {
        if (node[name])
        {
            std::string pathValue = node[name].as<std::string>();
            if (pathValue.empty()) {
                path = "";
                return;
            }

            std::filesystem::path fsPath(pathValue);
            
            // If path is already absolute - keep it as is
            if (fsPath.is_absolute()) {
                path = pathValue;
                #ifdef CH_PLATFORM_WINDOWS
                std::replace(path.begin(), path.end(), '\\', '/');
                std::transform(path.begin(), path.end(), path.begin(), ::tolower);
                #endif
                return;
            }

            // If relative - KEEP IT RELATIVE!
            // AssetManager will resolve it to absolute when loading
            path = pathValue;
            
            #ifdef CH_PLATFORM_WINDOWS
            std::replace(path.begin(), path.end(), '\\', '/');
            std::transform(path.begin(), path.end(), path.begin(), ::tolower);
            #endif
        }
    }

    inline void DeserializePath(YAML::Node node, const char* name, std::filesystem::path& path)
    {
        std::string pathStr;
        DeserializePath(node, name, pathStr);
        if (!pathStr.empty())
            path = pathStr;
    }

    // For optional components or complex types that might be missing
    template<typename T>
    inline bool HasProperty(YAML::Node node, const char* name)
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
        enum Mode { Serialize, Deserialize };

        // Serialize constructor
        PropertyArchive(YAML::Emitter& emitter) 
            : m_Mode(Serialize), m_Out(&emitter), m_Node() {}

        // Deserialize constructor
        PropertyArchive(YAML::Node node) 
            : m_Mode(Deserialize), m_Out(nullptr), m_Node(node) {}

        // Check if property exists (for backwards compatibility/migrations)
        bool HasProperty(const char* name)
        {
            if (m_Mode == Serialize) return true; // Always "has" property in serialize mode (conceptually) or return false? 
                                                  // Actually for migration checks we usually check if generic properties exist in READ mode.
            return m_Node[name].IsDefined();
        }

        // Generic property handler
        template<typename T>
        PropertyArchive& operator()(const char* name, T& value)
        {
            if (m_Mode == Serialize)
                SerializeProperty(*m_Out, name, value);
            else
                DeserializeProperty(m_Node, name, value);
            return *this;
        }

        // Fluent alias for property
        template<typename T>
        PropertyArchive& Property(const char* name, T& value)
        {
            return (*this)(name, value);
        }

        // Path property (handles relative/absolute conversion)
        PropertyArchive& Path(const char* name, std::string& path)
        {
            if (m_Mode == Serialize)
                SerializePath(*m_Out, name, path);
            else
                DeserializePath(m_Node, name, path);
            return *this;
        }

        PropertyArchive& Path(const char* name, std::filesystem::path& path)
        {
            if (m_Mode == Serialize)
                SerializePath(*m_Out, name, path.string());
            else
                DeserializePath(m_Node, name, path);
            return *this;
        }

        // Nested structure (TextStyle, UIStyle, etc.)
        template<typename T, typename SerFunc, typename DeserFunc>
        PropertyArchive& Nested(const char* name, T& value, SerFunc serializeFunc, DeserFunc deserializeFunc)
        {
            if (m_Mode == Serialize)
            {
                *m_Out << YAML::Key << name;
                *m_Out << YAML::BeginMap;
                serializeFunc(*m_Out, value);
                *m_Out << YAML::EndMap;
            }
            else
            {
                if (m_Node[name])
                    deserializeFunc(value, m_Node[name]);
            }
            return *this;
        }

        // Array/Sequence property
        template<typename T>
        PropertyArchive& Sequence(const char* name, std::vector<T>& vec)
        {
            if (m_Mode == Serialize)
            {
                *m_Out << YAML::Key << name << YAML::Value << YAML::BeginSeq;
                for (const auto& item : vec)
                    *m_Out << item;
                *m_Out << YAML::EndSeq;
            }
            else
            {
                if (m_Node[name] && m_Node[name].IsSequence())
                {
                    vec.clear();
                    for (auto item : m_Node[name])
                        vec.push_back(item.template as<T>());
                }
            }
            return *this;
        }

        Mode GetMode() const { return m_Mode; }

    private:
        Mode m_Mode;
        YAML::Emitter* m_Out;
        YAML::Node m_Node;
    };
}

#endif // CH_SERIALIZATION_UTILS_H
