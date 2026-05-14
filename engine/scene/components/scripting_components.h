#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H   
#include "engine/core/reflection.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
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
        CH_PROP(props, Name);
        CH_PROP_NAMED(props, "Type", (int&)Type);
        
        // Handle variant reflection
        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, CHEngine::Color>)
                CH_PROP_NAMED(props, "Value", val);
            else if constexpr (std::is_same_v<T, uint64_t>)
            {
                if (props.GetMode() != CHEngine::ReflectionMode::UI)
                    CH_HANDLE_NAMED(props, "Value", val);
            }
            else if constexpr (std::is_same_v<T, float>)
                CH_PROP_META_NAMED(props, "Value", val, PropertyMeta(-100.0f, 100.0f, 0.01f));
            else
                CH_PROP_NAMED(props, "Value", val);
        }, Value);
    CH_REFLECT_END()
};

// Represents a single C# script instance attached to an entity.
// Instance is stored as shared_ptr<void> with a type-erasing custom deleter set by
// SceneScriptingManager so that Coral headers do not need to be included here.
struct ManagedScriptInstance
{
    std::string ClassName;
    std::map<std::string, ScriptField> Fields; // Persistent fields

    // Owning smart pointer to the backing Coral::ManagedObject.
    // The deleter is injected by SceneScriptingManager to avoid including Coral headers here.
    std::shared_ptr<void> Instance;
    bool        NeedsStart   = true;

    // Engine calls methods directly via Coral::ManagedObject::InvokeMethod.

    ManagedScriptInstance() = default;
    explicit ManagedScriptInstance(const std::string& className)
        : ClassName(className) {}

    // Copy only persisted data — runtime state is never copied.
    ManagedScriptInstance(const ManagedScriptInstance& other)
        : ClassName(other.ClassName), Fields(other.Fields) {}
    ManagedScriptInstance& operator=(const ManagedScriptInstance& other)
    {
        if (this != &other)
        {
            ClassName = other.ClassName;
            Fields = other.Fields;
            Instance.reset();
            NeedsStart = true;
        }
        return *this;
    }

    // Move is allowed — transfers ownership of Instance.
    ManagedScriptInstance(ManagedScriptInstance&&) = default;
    ManagedScriptInstance& operator=(ManagedScriptInstance&&) = default;

    // Returns a raw (non-owning) pointer to the underlying object. Cast as needed.
    void* GetRaw() const { return Instance.get(); }
    bool HasInstance() const { return Instance != nullptr; }

    CH_REFLECT_BEGIN(ManagedScriptInstance)
        CH_PROP_META_NAMED(props, "ClassName", ClassName, PropertyMeta(PropertyMeta::WidgetHint::Enum));
        for (auto& [name, field] : Fields)
        {
            std::visit([&](auto&& val) {
                CH_PROP_NAMED(props, name.c_str(), val);
            }, field.Value);
        }
    CH_REFLECT_END()
};

// ECS component that enables managed (C#) scripting for an entity.
struct ManagedScriptComponent
{
    std::vector<ManagedScriptInstance> Scripts;
    ManagedScriptComponent() = default;

    CH_REFLECT_BEGIN(ManagedScriptComponent)
        CH_SEQUENCE_NAMED(props, "Scripts", Scripts);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_SCRIPTING_COMPONENTS_H
