#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H   
#include "engine/core/reflection.h"
#include "scripting/scriptengine.h"
#include <Coral/ManagedObject.hpp>
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
        
        // Sync fields with script engine definition
        if (props.GetMode() == ReflectionMode::UI)
        {
            auto* scriptType = ScriptEngine::Get().GetScriptClass(ClassName);
            if (scriptType)
            {
                auto fieldInfos = scriptType->GetFields();
                for (auto& info : fieldInfos)
                {
                    if (info.GetAccessibility() != Coral::TypeAccessibility::Public) continue;
                    std::string fieldName = (std::string)info.GetName();
                    
                    if (Fields.find(fieldName) == Fields.end())
                    {
                        // Add default field
                        ScriptField f;
                        f.Name = fieldName;
                        // ... setup default value based on type (simplified for now)
                        Fields[fieldName] = f;
                    }

                    auto& field = Fields[fieldName];
                    
                    // Reflect the field itself (which handles its name and variant value)
                    Properties<T_Archive> fieldProps(props.GetArchive());
                    field.Reflect(fieldProps);

                    if (fieldProps.HasChanged() && Instance)
                    {
                        auto* obj = static_cast<Coral::ManagedObject*>(Instance);
                        std::visit([&](auto&& v) { obj->SetFieldValue(fieldName, v); }, field.Value);
                    }
                }
            }
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
