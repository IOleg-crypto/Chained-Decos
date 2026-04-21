#ifndef CH_REFLECTION_H
#define CH_REFLECTION_H

#include <vector>
#include <string>
#include "engine/core/base.h"
#include "engine/core/assets/asset.h"
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
    struct PropertyMeta
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
            return m_Archive.Sequence(name, values, allowAddRemove);
        }

        template<typename T>
        bool Nested(const char* name, T& value)
        {
            return m_Archive.Nested(name, value);
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
            bool changed = m_Archive.Property(name, temp, names, count);
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
            bool changed = m_Archive.Property(name, temp, names, count, meta);
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
            if constexpr (std::is_same_v<decltype(m_Archive.Action(label, func)), void>)
            {
                m_Archive.Action(label, func);
                return false;
            }
            else
            {
                return m_Archive.Action(label, func);
            }
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

    // --- Reflection Macros ---

    #define CH_REFLECT_BEGIN(Type) \
        static const char* GetStaticName() { return #Type; } \
        template<typename T_Archive> \
        void Reflect(::CHEngine::Properties<T_Archive>& props) {

    #define CH_REFLECT_END() \
        }
}
#endif // CH_REFLECTION_H
