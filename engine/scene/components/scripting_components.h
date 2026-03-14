#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H   
#include <vector>
#include <string>
#include <map>
#include <variant>
#include "raylib.h"

namespace CHEngine
{

enum class ScriptFieldType
{
    None = 0,
    Float, Int, Bool, String, Vector2, Vector3, Vector4, Color, Entity
};

struct ScriptField
{
    ScriptFieldType Type = ScriptFieldType::None;
    std::string Name;
    std::variant<float, int, bool, std::string, Vector2, Vector3, Vector4, Color, uint64_t> Value;
};

// Represents a single C# script instance attached to an entity.
struct ManagedScriptInstance
{
    std::string ClassName;    // Full qualified name, e.g. "ChainedDecos.Scripts.PlayerController"
    std::map<std::string, ScriptField> Fields; // Persistent fields

    void*       Instance     = nullptr; // Runtime only: Coral::ManagedObject*
    bool        NeedsStart   = true;    // True until OnStart() has been invoked

    // High-performance lifecycle delegates
    void (*OnCreate)()        = nullptr;
    void (*OnStart)()         = nullptr;
    void (*OnUpdate)(float)   = nullptr;
    void (*OnDestroy)()       = nullptr;

    ManagedScriptInstance() = default;
    explicit ManagedScriptInstance(const std::string& className)
        : ClassName(className) {}
};

// ECS component that enables managed (C#) scripting for an entity.
struct ManagedScriptComponent
{
    std::vector<ManagedScriptInstance> Scripts;
    ManagedScriptComponent() = default;
};

} // namespace CHEngine

#endif // CH_SCRIPTING_COMPONENTS_H
