#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H   
#include "engine/core/reflection.h"
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <glm/glm.hpp>

namespace CHEngine
{

enum class ScriptFieldType
{
    None = 0,
    Float, Int, Bool, String, Vec2, Vec3, Vec4, Color, Entity
};

struct ScriptField
{
    ScriptFieldType Type = ScriptFieldType::None;
    std::string Name;
    std::variant<float, int, bool, std::string, glm::vec2, glm::vec3, glm::vec4, CHEngine::Color, uint64_t> Value;

    CH_REFLECT_BEGIN(ScriptField)
        props.Property("Name", Name);
        props.Property("Type", (int&)Type);
        
        // Handle variant reflection
        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CHEngine::Color>)
                props.Color("Value", val);
            else if constexpr (std::is_same_v<T, uint64_t>)
                props.Handle("Value", val);
            else
                props.Property("Value", val);
        }, Value);
    CH_REFLECT_END()
};

// Represents a single C# script instance attached to an entity.
struct ManagedScriptInstance
{
    std::string ClassName;
    std::map<std::string, ScriptField> Fields; // Persistent fields

    void*       Instance     = nullptr;
    bool        NeedsStart   = true;

    // High-performance lifecycle delegates
    void (*OnCreate)()        = nullptr;
    void (*OnStart)()         = nullptr;
    void (*OnUpdate)(float)   = nullptr;
    void (*OnDestroy)()       = nullptr;
    void (*OnGUI)()           = nullptr;
    void (*OnCollisionEnter)(uint64_t) = nullptr;

    ManagedScriptInstance() = default;
    explicit ManagedScriptInstance(const std::string& className)
        : ClassName(className) {}

    CH_REFLECT_BEGIN(ManagedScriptInstance)
        props.Property("ClassName", ClassName);
        for (auto& [name, field] : Fields)
        {
            props.Nested(name.c_str(), field);
        }
    CH_REFLECT_END()
};

// ECS component that enables managed (C#) scripting for an entity.
struct ManagedScriptComponent
{
    std::vector<ManagedScriptInstance> Scripts;
    ManagedScriptComponent() = default;

    CH_REFLECT_BEGIN(ManagedScriptComponent)
        props.Sequence("Scripts", Scripts);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_SCRIPTING_COMPONENTS_H
