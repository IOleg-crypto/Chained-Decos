#ifndef CH_REFLECTION_RFL_H
#define CH_REFLECTION_RFL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <array>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <rfl/to_view.hpp>
#include <type_traits>
#include <utility>
#include "engine/foundation/uuid.h"
#include "engine/reflection/reflection.h"

// reflect-cpp extensions for Chained Engine

namespace Chained
{

/**
 * @brief Bridge that allows reflect-cpp types to be used with the legacy Properties system.
 * This is useful for maintaining backward compatibility with the current Inspector and Serializers
 * while moving away from manual CH_REFLECT macros.
 */
template<typename T, typename T_Archive>
void ReflectFromRfl(T& component, Chained::Properties<T_Archive>& props)
{
    auto view = rfl::to_view(component);
    
    view.apply([&](auto field) {
        using FieldType = std::decay_t<decltype(*field.get())>;

        std::string name(field.name());
        

        // Skip known runtime and internal state fields that should not be serialized or displayed
        if (name == "WorldTransform" || name == "InverseWorldTransform" || name == "RotationQuat" || 
            name == "IsDirty" || name == "Parent" || name == "PrevTranslation" || 
            name == "PrevRotationQuat" || name == "PrevScale" || name == "IsColliding" || 
            name == "Handle" || name == "Velocity" || name == "IsGrounded" || name == "State" ||
            name == "ModelHandle" || name == "MaterialsInitialized")
        {
            return;
        }

        if constexpr (is_vector_v<FieldType>)
        {
            props.Sequence(name.c_str(), *field.get());
        }
        else if constexpr (std::is_same_v<FieldType, int> ||
                      std::is_same_v<FieldType, float> ||
                      std::is_same_v<FieldType, bool> ||
                      std::is_same_v<FieldType, std::string> ||
                      std::is_same_v<FieldType, glm::vec2> ||
                      std::is_same_v<FieldType, glm::vec3> ||
                      std::is_same_v<FieldType, glm::vec4> ||
                      std::is_same_v<FieldType, Chained::Color> ||
                      std::is_same_v<FieldType, uint64_t> ||
                      std::is_enum_v<FieldType> ||
                      (std::is_integral_v<FieldType> && std::is_unsigned_v<FieldType>))
        {
            props.Property(name.c_str(), *field.get());
        }
        else if constexpr (is_rfl_component<FieldType>::value)
        {
            props.GetArchive().Nested(name.c_str(), [&](IPropertyArchive& archive) {
                Properties<IPropertyArchive> nested(archive);
                ReflectFromRfl(*field.get(), nested);
            });
        }
        else if constexpr (requires(FieldType& value, Chained::Properties<T_Archive>& p) { value.Reflect(p); })
        {
            if (props.Nested(name.c_str(), *field.get()))
            {
                // Nested objects report their own changes through the archive.
            }
        }
        // Skip unsupported types for now to avoid build errors
    });
}

// Macro to mark a component as reflectable via reflect-cpp without manual Reflect() method.
#define CH_MARK_RFL(Type) \
    template<> struct is_rfl_component<Type> : std::true_type {};

/**
 * @brief Proxy structs to allow reflect-cpp to handle non-aggregate types.
 */
    struct UIMeta {
        float Min = 0.0f;
        float Max = 0.0f;
        float Speed = 0.1f;
        const char* Tooltip = nullptr;
    };

    struct Vec3Proxy {
        float x, y, z;
        static Vec3Proxy from_class(const glm::vec3& v) { return {v.x, v.y, v.z}; }
        glm::vec3 to_class() const { return glm::vec3(x, y, z); }
    };

    struct QuatProxy {
        float w, x, y, z;
        static QuatProxy from_class(const glm::quat& q) { return {q.w, q.x, q.y, q.z}; }
        glm::quat to_class() const { return glm::quat(w, x, y, z); }
    };

    struct UUIDProxy {
        uint64_t value;
        static UUIDProxy from_class(const Chained::UUID& u) { return { static_cast<uint64_t>(u) }; }
        Chained::UUID to_class() const { return Chained::UUID(value); }
    };

    struct ColorProxy {
        unsigned char r, g, b, a;
        static ColorProxy from_class(const Chained::Color& c) { return {c.r, c.g, c.b, c.a}; }
        Chained::Color to_class() const { return Chained::Color(r, g, b, a); }
    };
}

namespace rfl::parsing {
    template <class R, class W, class ProcessorsType>
    struct Parser<R, W, glm::vec3, ProcessorsType> 
        : public CustomParser<R, W, ProcessorsType, glm::vec3, Chained::Vec3Proxy> {};

    template <class R, class W, class ProcessorsType>
    struct Parser<R, W, glm::quat, ProcessorsType> 
        : public CustomParser<R, W, ProcessorsType, glm::quat, Chained::QuatProxy> {};

    template <class R, class W, class ProcessorsType>
    struct Parser<R, W, Chained::UUID, ProcessorsType> 
        : public CustomParser<R, W, ProcessorsType, Chained::UUID, Chained::UUIDProxy> {};

    template <class R, class W, class ProcessorsType>
    struct Parser<R, W, Chained::Color, ProcessorsType> 
        : public CustomParser<R, W, ProcessorsType, Chained::Color, Chained::ColorProxy> {};
}

#endif // CH_REFLECTION_RFL_H
