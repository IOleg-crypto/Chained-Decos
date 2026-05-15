#ifndef CH_REFLECTION_H
#define CH_REFLECTION_H

#include <vector>
#include <string>
#include "engine/core/base.h"
#include "engine/assets/asset.h"
#include <functional>
#include <type_traits>

#define CH_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#include <variant>
#include <type_traits>

namespace CHEngine
{
    // Helper to detect std::variant
    template <typename T> struct is_variant : std::false_type {};
    template <typename... Args> struct is_variant<std::variant<Args...>> : std::true_type {};
    template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;
    enum class ReflectionMode
    {
        None = 0,
        Serialize,
        Deserialize,
        UI
    };

    // Widget rendering hints for UI properties
    struct CH_API PropertyMeta
    {
        enum class WidgetHint
        {
            Default,    // Auto-select based on type
            Slider,     // Use slider instead of input
            Input,      // Use text input
            Checkbox,   // Force checkbox
            ColorPicker, // Color picker
            FilePicker, // File picker
            Enum        // Enum dropdown
        } Hint = WidgetHint::Default;

        float MinValue = 0.0f;
        float MaxValue = 0.0f;
        float Speed = 0.1f;
        std::string Tooltip;
        bool ReadOnly = false;

        // Convenience constructors
        PropertyMeta() = default;
        PropertyMeta(WidgetHint h) : Hint(h) {}
        PropertyMeta(float min, float max, float spd = 0.1f)
            : Hint(WidgetHint::Slider), MinValue(min), MaxValue(max), Speed(spd) {}

        PropertyMeta& WithTooltip(const std::string& t) { Tooltip = t; return *this; }
    };

    class IPropertyArchive;
    template<typename T_Archive> class Properties;
    
    /**
     * @brief Interface for any archive that can handle reflected properties.
     */
    class CH_API IPropertyArchive
    {
    public:
        virtual ~IPropertyArchive() = default;
        virtual ReflectionMode GetReflectionMode() const = 0;
        virtual bool Property(const char* name, int& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, float& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, bool& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, std::string& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, glm::vec2& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, glm::vec3& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, glm::vec4& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, uint64_t& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Property(const char* name, Color& value, const PropertyMeta& meta = {}) = 0;
        virtual bool Enum(const char* name, int& value, const char** names, int count, const PropertyMeta& meta = {}) = 0;
        virtual bool Handle(const char* name, uint64_t& value, const PropertyMeta& meta = {}) = 0;
        virtual bool File(const char* name, std::string& value, const char* extensions = nullptr, const PropertyMeta& meta = {}) = 0;
        virtual bool Action(const char* label, std::function<void()> func) = 0;
        virtual void Header(const char* label) = 0;
        virtual void Separator() = 0;
        virtual bool BeginGroup(const char* label, bool defaultOpen = true) = 0;
        virtual void EndGroup() = 0;
        virtual bool HasChanged() const = 0;
        virtual void SetChanged(bool changed) = 0;
        virtual void BeginSequence(const char* name, size_t& size) = 0;
        virtual void EndSequence() = 0;
        virtual bool Nested(const char* name, std::function<void(IPropertyArchive&)> callback) = 0;
    };

    // Alias moved to end of file to resolve circular dependencies

    // The "Properties" class is the primary interface for reflection.
    // It is used by both Serializers (YAML) and Editor UI (ImGui).
    template<typename T_Archive>
    class Properties
    {
    public:
        Properties(T_Archive& archive) : m_Archive(archive) {}

        ReflectionMode GetMode() const { return m_Archive.GetReflectionMode(); }

        bool Color(const char* name, CHEngine::Color& value)
        {
            if constexpr (std::is_base_of_v<IPropertyArchive, T_Archive>)
                return m_Archive.Property(name, value);
            else
                return m_Archive.Property(name, value);
        }

        bool Handle(const char* name, uint64_t& value)
        {
            return m_Archive.Handle(name, value);
        }

        bool Handle(const char* name, CHEngine::UUID& value)
        {
            return m_Archive.Handle(name, (uint64_t&)value);
        }

        template<typename T>
        bool Sequence(const char* name, std::vector<T>& values, bool allowAddRemove = true)
        {
            size_t size = values.size();
            m_Archive.BeginSequence(name, size);
            
            if (GetMode() == ReflectionMode::Deserialize)
            {
                values.resize(size);
            }

            bool changed = false;
            for (size_t i = 0; i < values.size(); ++i)
            {
                std::string label = "[" + std::to_string(i) + "]";
                if (m_Archive.GetReflectionMode() == ReflectionMode::UI)
                {
                    if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T>)
                    {
                        if (m_Archive.Property(label.c_str(), values[i])) changed = true;
                    }
                    else
                    {
                        if (Nested(label.c_str(), values[i])) changed = true;
                    }
                }
                else
                {
                    // For serialization/deserialization, we don't want keys for sequence items
                    if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T>)
                    {
                        if (m_Archive.Property(nullptr, values[i])) changed = true;
                    }
                    else
                    {
                        if (m_Archive.Nested(nullptr, [&](IPropertyArchive& archive) {
                            Properties<IPropertyArchive> props(archive);
                            values[i].Reflect(props);
                        })) changed = true;
                    }
                }
            }

            m_Archive.EndSequence();
            return changed;
        }

        template<typename T>
        bool Nested(const char* name, T& value)
        {
            if constexpr (std::is_base_of_v<IPropertyArchive, T_Archive>)
            {
                return m_Archive.Nested(name, [&](IPropertyArchive& archive) {
                    Properties<IPropertyArchive> props(archive);
                    value.Reflect(props);
                });
            }
            else
            {
                return m_Archive.Nested(name, value);
            }
        }

        template<typename T>
        bool Property(const char* name, T& value)
        {
            if constexpr (is_variant_v<T>)
            {
                return std::visit([&](auto&& v) {
                    return m_Archive.Property(name, v);
                }, value);
            }
            else
            {
                return m_Archive.Property(name, value);
            }
        }

        template<typename T_Enum>
        bool Enum(const char* name, T_Enum& value, const char** names, int count)
        {
            int temp = (int)value;
            bool changed = m_Archive.Enum(name, temp, names, count);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = (T_Enum)temp;
            }
            return changed;
        }

        template<typename T_Enum>
        bool Property(const char* name, T_Enum& value, const char** names, int count)
        {
            return Enum(name, value, names, count);
        }

        bool File(const char* name, std::string& value, const char* extensions = nullptr)
        {
            return m_Archive.File(name, value, extensions);
        }

        // --- Property methods with metadata ---
        template<typename T>
        bool Property(const char* name, T& value, const PropertyMeta& meta)
        {
            if constexpr (is_variant_v<T>)
            {
                return std::visit([&](auto&& v) {
                    return m_Archive.Property(name, v, meta);
                }, value);
            }
            else
            {
                return m_Archive.Property(name, value, meta);
            }
        }

        template<typename T_Enum>
        bool Enum(const char* name, T_Enum& value, const char** names, int count, const PropertyMeta& meta)
        {
            int temp = (int)value;
            bool changed = m_Archive.Enum(name, temp, names, count, meta);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = (T_Enum)temp;
            }
            return changed;
        }

        bool File(const char* name, std::string& value, const char* extensions, const PropertyMeta& meta)
        {
            return m_Archive.File(name, value, extensions, meta);
        }

        bool Action(const char* label, std::function<void()> func)
        {
            return m_Archive.Action(label, func);
        }

        void Header(const char* label)
        {
            m_Archive.Header(label);
        }

        void Separator()
        {
            m_Archive.Separator();
        }

        bool BeginGroup(const char* label, bool defaultOpen = true)
        {
            return m_Archive.BeginGroup(label, defaultOpen);
        }

        void EndGroup()
        {
            m_Archive.EndGroup();
        }

        // Post-change hook
        Properties& OnChange(std::function<void()> func)
        {
            if (m_Archive.HasChanged())
            {
                func();
            }
            return *this;
        }

        void SetChanged(bool changed) { m_Archive.SetChanged(changed); }
        bool HasChanged() const { return m_Archive.HasChanged(); }
        T_Archive& GetArchive() { return m_Archive; }

    private:
        T_Archive& m_Archive;
    };

    /**
     * @brief Specialized properties for when the archive type is erased.
     */
    using GenericProperties = Properties<IPropertyArchive>;

    // --- Reflection Macros ---

    #define CH_REFLECT_BEGIN(Type) \
        static const char* GetStaticName() { return #Type; } \
        template<typename T_Archive> \
        void Reflect(::CHEngine::Properties<T_Archive>& props) {

    #define CH_REFLECT_END() \
        }

    #define CH_HEADER(props, label) (props).Header(label)
    #define CH_SEPARATOR(props) (props).Separator()
    #define CH_PROP(props, field) (props).Property(#field, field)
    #define CH_PROP_NAMED(props, label, field) (props).Property(label, field)
    #define CH_PROP_META(props, field, meta) (props).Property(#field, field, meta)
    #define CH_PROP_META_NAMED(props, label, field, meta) (props).Property(label, field, meta)
    #define CH_ENUM(props, field, names) (props).Enum(#field, field, names, static_cast<int>(CH_ARRAY_SIZE(names)))
    #define CH_ENUM_NAMED(props, label, field, names) (props).Enum(label, field, names, static_cast<int>(CH_ARRAY_SIZE(names)))
    #define CH_HANDLE(props, field) (props).Handle(#field, field)
    #define CH_HANDLE_NAMED(props, label, field) (props).Handle(label, field)
    #define CH_FILE(props, field, extensions) (props).File(#field, field, extensions)
    #define CH_FILE_NAMED(props, label, field, extensions) (props).File(label, field, extensions)
    #define CH_SEQUENCE(props, field) (props).Sequence(#field, field)
    #define CH_SEQUENCE_EX(props, field, allowAddRemove) (props).Sequence(#field, field, allowAddRemove)
    #define CH_SEQUENCE_NAMED(props, label, field) (props).Sequence(label, field)
    #define CH_NESTED(props, field) (props).Nested(#field, field)
    #define CH_NESTED_NAMED(props, label, field) (props).Nested(label, field)
    #define CH_ACTION(props, label, func) (props).Action(label, func)
    #define CH_BEGIN_GROUP(props, label, defaultOpen) (props).BeginGroup(label, defaultOpen)
    #define CH_END_GROUP(props) (props).EndGroup()
}
#endif // CH_REFLECTION_H
