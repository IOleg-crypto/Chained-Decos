#ifndef CH_REFLECTION_RFL_H
#define CH_REFLECTION_RFL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <rfl.hpp>
#include <rfl/to_view.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <string_view>
#include <type_traits>

#include "engine/common/uuid.h"
#include "engine/reflection/reflection.h"

namespace Chained
{




struct UIMeta
{
    PropertyMeta::WidgetHint Hint = PropertyMeta::WidgetHint::Default;
    float Min = 0.0f;
    float Max = 0.0f;
    float Speed = 0.1f;
    bool ReadOnly = false;
    bool Transient = false;
    const char* Tooltip = nullptr;
    const char* Extensions = nullptr; 
};




struct AssetPath
{
    std::string Path;
    const char* Extensions = "*.*";

    AssetPath() = default;
    AssetPath(const std::string& path, const char* ext = "*.*")
        : Path(path),
          Extensions(ext)
    {
    }
};




struct ExtractedFieldMeta
{
    PropertyMeta Meta;
    const char* Extensions = nullptr;
};





template <typename T_Component> ExtractedFieldMeta GetMetaForField(std::string_view field_name)
{
    ExtractedFieldMeta result;

    if constexpr (requires { typename T_Component::UI; })
    {
        using UI_Struct = typename T_Component::UI;
        UI_Struct ui_layout{};
        auto ui_view = rfl::to_view(ui_layout);

        ui_view.apply([&](auto ui_field) {
            if (ui_field.name() == field_name)
            {
                const UIMeta& src = *ui_field.get();
                result.Meta.Hint = src.Hint;
                result.Meta.MinValue = src.Min;
                result.Meta.MaxValue = src.Max;
                result.Meta.Speed = src.Speed;
                result.Meta.ReadOnly = src.ReadOnly;
                result.Meta.Transient = src.Transient;
                result.Extensions = src.Extensions; 

                if (src.Tooltip)
                {
                    result.Meta.Tooltip = src.Tooltip;
                }

                
                if (src.Min != src.Max && src.Hint == PropertyMeta::WidgetHint::Default)
                {
                    result.Meta.Hint = PropertyMeta::WidgetHint::Slider;
                }
            }
        });
    }
    return result;
}




template <typename T, typename T_Archive> void ReflectFromRfl(T& component, Chained::Properties<T_Archive>& props)
{
    auto view = rfl::to_view(component);

    view.apply([&](auto field) {
        using FieldType = std::decay_t<decltype(*field.get())>;
        std::string_view name = field.name();



        
        ExtractedFieldMeta field_meta = GetMetaForField<T>(name);
        PropertyMeta& meta = field_meta.Meta;

        
        if (meta.Transient && props.GetMode() != ReflectionMode::UI)
        {
            return;
        }

        const char* name_cstr = field.name().data();

        
        if constexpr (is_vector_v<FieldType>)
        {
            props.Sequence(name_cstr, *field.get());
        }
        
        else if constexpr (std::is_same_v<FieldType, Chained::AssetPath>)
        {
            props.GetArchive().File(name_cstr, field.get()->Path, field.get()->Extensions, meta);
        }
        
        else if constexpr (std::is_same_v<FieldType, std::string>)
        {
            if (meta.Hint == PropertyMeta::WidgetHint::FilePicker)
            {
                
                props.GetArchive().File(name_cstr, *field.get(), field_meta.Extensions, meta);
            }
            else
            {
                props.Property(name_cstr, *field.get(), meta);
            }
        }
        
        else if constexpr (std::is_enum_v<FieldType>)
        {
            if (props.GetMode() == ReflectionMode::UI && meta.Hint == PropertyMeta::WidgetHint::Enum)
            {
                constexpr auto enumerators = rfl::get_enumerator_array<FieldType>();
                std::vector<std::string> names_str;
                std::vector<const char*> names_cstr;
                names_str.reserve(enumerators.size());
                for (const auto& p : enumerators)
                    names_str.emplace_back(p.first);
                names_cstr.reserve(names_str.size());
                for (const auto& s : names_str)
                    names_cstr.push_back(s.c_str());
                props.Enum(name_cstr, *field.get(), names_cstr.data(), (int)names_cstr.size(), meta);
            }
            else
            {
                props.Property(name_cstr, *field.get(), meta);
            }
        }
        else if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, float> ||
                           std::is_same_v<FieldType, bool> || std::is_same_v<FieldType, glm::vec2> ||
                           std::is_same_v<FieldType, glm::vec3> || std::is_same_v<FieldType, glm::vec4> ||
                           std::is_same_v<FieldType, Chained::Color> || std::is_same_v<FieldType, uint64_t> ||
                           is_variant_v<FieldType> ||
                           (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>))
        {
            props.Property(name_cstr, *field.get(), meta);
        }
        
        else if constexpr (is_rfl_component<FieldType>::value)
        {
            props.GetArchive().Nested(name_cstr, [&](IPropertyArchiveBase& archive) {
                Properties<IPropertyArchiveBase> nested(archive);
                ReflectFromRfl(*field.get(), nested);
            });
        }
        
        else if constexpr (requires(FieldType& value, Chained::Properties<T_Archive>& p) { value.Reflect(p); })
        {
            props.Nested(name_cstr, *field.get());
        }
    });
}

#define CH_MARK_RFL(Type)                                                                                              \
    template <> struct is_rfl_component<Type> : std::true_type                                                         \
    {                                                                                                                  \
    };

#define PROPERTY(Type, Name) rfl::field<#Name, Type> Name


struct Vec3Proxy
{
    float x, y, z;
    static Vec3Proxy from_class(const glm::vec3& v)
    {
        return {v.x, v.y, v.z};
    }
    glm::vec3 to_class() const
    {
        return glm::vec3(x, y, z);
    }
};

struct QuatProxy
{
    float w, x, y, z;
    static QuatProxy from_class(const glm::quat& q)
    {
        return {q.w, q.x, q.y, q.z};
    }
    glm::quat to_class() const
    {
        return glm::quat(w, x, y, z);
    }
};

struct UUIDProxy
{
    uint64_t value;
    static UUIDProxy from_class(const Chained::UUID& u)
    {
        return {static_cast<uint64_t>(u)};
    }
    Chained::UUID to_class() const
    {
        return Chained::UUID(value);
    }
};

struct ColorProxy
{
    unsigned char r, g, b, a;
    static ColorProxy from_class(const Chained::Color& c)
    {
        return {c.r, c.g, c.b, c.a};
    }
    Chained::Color to_class() const
    {
        return Chained::Color(r, g, b, a);
    }
};
} // namespace Chained


namespace rfl::parsing
{
template <class R, class W, class ProcessorsType>
struct Parser<R, W, glm::vec3, ProcessorsType>
    : public CustomParser<R, W, ProcessorsType, glm::vec3, Chained::Vec3Proxy>
{
};

template <class R, class W, class ProcessorsType>
struct Parser<R, W, glm::quat, ProcessorsType>
    : public CustomParser<R, W, ProcessorsType, glm::quat, Chained::QuatProxy>
{
};

template <class R, class W, class ProcessorsType>
struct Parser<R, W, Chained::UUID, ProcessorsType>
    : public CustomParser<R, W, ProcessorsType, Chained::UUID, Chained::UUIDProxy>
{
};

template <class R, class W, class ProcessorsType>
struct Parser<R, W, Chained::Color, ProcessorsType>
    : public CustomParser<R, W, ProcessorsType, Chained::Color, Chained::ColorProxy>
{
};


struct AssetPathProxy
{
    std::string value;
    static AssetPathProxy from_class(const Chained::AssetPath& p)
    {
        return {p.Path};
    }
    Chained::AssetPath to_class() const
    {
        return {value, "*.*"};
    }
};

template <class R, class W, class ProcessorsType>
struct Parser<R, W, Chained::AssetPath, ProcessorsType>
    : public CustomParser<R, W, ProcessorsType, Chained::AssetPath, AssetPathProxy>
{
};
} // namespace rfl::parsing

#endif // CH_REFLECTION_RFL_H