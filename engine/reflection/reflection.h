#ifndef CH_REFLECTION_H
#define CH_REFLECTION_H

#include "engine/common/base.h"
#include "engine/common/color.h"
#include "engine/common/uuid.h"
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <type_traits>
#include <vector>
#include <type_traits>
#include <variant>


namespace Chained
{
// Helper to detect std::variant
template <typename T> struct is_variant : std::false_type
{
};
template <typename... Args> struct is_variant<std::variant<Args...>> : std::true_type
{
};
template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;
template <typename T> struct is_rfl_component : std::false_type
{
};

template <typename T> struct is_vector : std::false_type
{
};
template <typename T, typename Alloc> struct is_vector<std::vector<T, Alloc>> : std::true_type
{
    using value_type = T;
};
template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;
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
        Default,     // Auto-select based on type
        Slider,      // Use slider instead of input
        Input,       // Use text input
        Checkbox,    // Force checkbox
        ColorPicker, // Color picker
        FilePicker,  // File picker
        Enum         // Enum dropdown
    } Hint = WidgetHint::Default;

    float MinValue = 0.0f;
    float MaxValue = 0.0f;
    float Speed = 0.1f;
    std::string Tooltip;
    bool ReadOnly = false;
    bool Transient = false;

    // Convenience constructors
    PropertyMeta() = default;
    PropertyMeta(WidgetHint h)
        : Hint(h)
    {
    }
    PropertyMeta(float min, float max, float spd = 0.1f)
        : Hint(WidgetHint::Slider),
          MinValue(min),
          MaxValue(max),
          Speed(spd)
    {
    }

    PropertyMeta& WithTooltip(const std::string& t)
    {
        Tooltip = t;
        return *this;
    }
};

class IPropertyArchiveBase;
class IPropertyArchive;
template <typename T_Archive> class Properties;

template <typename T, typename T_Archive> void ReflectFromRfl(T& component, Chained::Properties<T_Archive>& props);

/**
 * @brief Base interface for property archives — data serialization and composition only.
 */
class CH_API IPropertyArchiveBase
{
public:
    virtual ~IPropertyArchiveBase() = default;
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
    virtual bool StringEnum(const char* name, std::string& value, const std::vector<std::string>& options,
                            const PropertyMeta& meta = {}) = 0;
    virtual bool Handle(const char* name, uint64_t& value, const PropertyMeta& meta = {}) = 0;
    virtual bool File(const char* name, std::string& value, const char* extensions = nullptr,
                      const PropertyMeta& meta = {}) = 0;
    virtual void BeginSequence(const char* name, size_t& size) = 0;
    virtual void EndSequence() = 0;
    virtual bool Nested(const char* name, std::function<void(IPropertyArchiveBase&)> callback) = 0;
};

/**
 * @brief Extended interface adding UI layout and change-tracking methods.
 */
class CH_API IPropertyArchive : public IPropertyArchiveBase
{
public:
    virtual ~IPropertyArchive() = default;
    virtual bool Action(const char* label, std::function<void()> func) = 0;
    virtual void Header(const char* label) = 0;
    virtual void Separator() = 0;
    virtual bool BeginGroup(const char* label, bool defaultOpen = true) = 0;
    virtual void EndGroup() = 0;
    virtual bool HasChanged() const = 0;
    virtual void SetChanged(bool changed) = 0;
};

// Alias moved to end of file to resolve circular dependencies

// The "Properties" class is the primary interface for reflection.
// It is used by both Serializers (YAML) and Editor UI (ImGui).
template <typename T_Archive> class Properties
{
public:
    Properties(T_Archive& archive)
        : m_Archive(archive)
    {
    }

    ReflectionMode GetMode() const
    {
        return m_Archive.GetReflectionMode();
    }

    bool Color(const char* name, Chained::Color& value)
    {
        return m_Archive.Property(name, value);
    }

    bool Handle(const char* name, uint64_t& value)
    {
        return m_Archive.Handle(name, value);
    }

    bool Handle(const char* name, Chained::UUID& value)
    {
        return m_Archive.Handle(name, (uint64_t&)value);
    }

    template <typename T> bool Sequence(const char* name, std::vector<T>& values, bool allowAddRemove = true)
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
                if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T> ||
                              std::is_enum_v<T> || (std::is_integral_v<T> && std::is_unsigned_v<T>))
                {
                    if (Property(label.c_str(), values[i]))
                    {
                        changed = true;
                    }
                }
                else if constexpr (is_rfl_component<T>::value)
                {
                    if (m_Archive.Nested(label.c_str(), [&](IPropertyArchiveBase& archive) {
                            Properties<IPropertyArchiveBase> props(archive);
                            ReflectFromRfl(values[i], props);
                        }))
                    {
                        changed = true;
                    }
                }
                else
                {
                    if (Nested(label.c_str(), values[i]))
                    {
                        changed = true;
                    }
                }
            }
            else
            {
                // For serialization/deserialization, we don't want keys for sequence items
                if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> || is_variant_v<T> ||
                              std::is_enum_v<T> || (std::is_integral_v<T> && std::is_unsigned_v<T>))
                {
                    if (Property(nullptr, values[i]))
                    {
                        changed = true;
                    }
                }
                else if constexpr (is_rfl_component<T>::value)
                {
                    if (m_Archive.Nested(nullptr, [&](IPropertyArchiveBase& archive) {
                            Properties<IPropertyArchiveBase> props(archive);
                            ReflectFromRfl(values[i], props);
                        }))
                    {
                        changed = true;
                    }
                }
                else
                {
                    if (m_Archive.Nested(nullptr, [&](IPropertyArchiveBase& archive) {
                            Properties<IPropertyArchiveBase> props(archive);
                            values[i].Reflect(props);
                        }))
                    {
                        changed = true;
                    }
                }
            }
        }

        m_Archive.EndSequence();
        return changed;
    }

    template <typename T> bool Nested(const char* name, T& value)
    {
        return m_Archive.Nested(name, [&](IPropertyArchiveBase& archive) {
            Properties<IPropertyArchiveBase> props(archive);
            value.Reflect(props);
        });
    }

    template <typename T> bool Property(const char* name, T& value)
    {
        if constexpr (is_variant_v<T>)
        {
            return std::visit([&](auto&& v) { return m_Archive.Property(name, v); }, value);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            int temp = (int)value;
            bool changed = m_Archive.Property(name, temp);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = (T)temp;
            }
            return changed;
        }
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, uint64_t> &&
                           !std::is_same_v<T, bool>)
        {
            uint64_t temp = static_cast<uint64_t>(value);
            bool changed = m_Archive.Property(name, temp);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = static_cast<T>(temp);
            }
            return changed;
        }
        else
        {
            return m_Archive.Property(name, value);
        }
    }

    template <typename T_Enum> bool Enum(const char* name, T_Enum& value, const char** names, int count)
    {
        int temp = (int)value;
        bool changed = m_Archive.Enum(name, temp, names, count);
        if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
        {
            value = (T_Enum)temp;
        }
        return changed;
    }

    template <typename T_Enum> bool Property(const char* name, T_Enum& value, const char** names, int count)
    {
        return Enum(name, value, names, count);
    }

    bool File(const char* name, std::string& value, const char* extensions = nullptr)
    {
        return m_Archive.File(name, value, extensions);
    }

    bool StringEnum(const char* name, std::string& value, const std::vector<std::string>& options,
                    const PropertyMeta& meta = {})
    {
        return m_Archive.StringEnum(name, value, options, meta);
    }

    // --- Property methods with metadata ---
    template <typename T> bool Property(const char* name, T& value, const PropertyMeta& meta)
    {
        if constexpr (is_variant_v<T>)
        {
            return std::visit([&](auto&& v) {
                if constexpr (requires { m_Archive.Property(name, v, meta); })
                {
                    return m_Archive.Property(name, v, meta);
                }
                else
                {
                    return false;
                }
            }, value);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            int temp = (int)value;
            bool changed = m_Archive.Property(name, temp, meta);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = (T)temp;
            }
            return changed;
        }
        else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, uint64_t> &&
                           !std::is_same_v<T, bool>)
        {
            uint64_t temp = static_cast<uint64_t>(value);
            bool changed = m_Archive.Property(name, temp, meta);
            if (changed || m_Archive.GetReflectionMode() == ReflectionMode::Deserialize)
            {
                value = static_cast<T>(temp);
            }
            return changed;
        }
        else
        {
            return m_Archive.Property(name, value, meta);
        }
    }

    template <typename T_Enum>
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

    void SetChanged(bool changed)
    {
        m_Archive.SetChanged(changed);
    }
    bool HasChanged() const
    {
        return m_Archive.HasChanged();
    }
    T_Archive& GetArchive()
    {
        return m_Archive;
    }

private:
    T_Archive& m_Archive;
};

/**
 * @brief Specialized properties for when the archive type is erased.
 */
using GenericProperties = Properties<IPropertyArchiveBase>;

} // namespace Chained
#endif // CH_REFLECTION_H
