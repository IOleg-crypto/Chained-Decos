#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H   
#include "engine/core/reflection_rfl.h"
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
};

CH_MARK_RFL(ScriptField);

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

    ManagedScriptInstance ClonePersistent() const
    {
        ManagedScriptInstance copy;
        copy.ClassName = ClassName;
        copy.Fields = Fields;
        return copy;
    }

    void ResetRuntimeState()
    {
        Instance.reset();
        NeedsStart = true;
    }

    // Returns a raw (non-owning) pointer to the underlying object. Cast as needed.
    void* GetRaw() const { return Instance.get(); }
    bool HasInstance() const { return Instance != nullptr; }
};

CH_MARK_RFL(ManagedScriptInstance);

// ECS component that enables managed (C#) scripting for an entity.
struct ManagedScriptComponent
{
    std::vector<ManagedScriptInstance> Scripts;

    ManagedScriptComponent ClonePersistent() const
    {
        ManagedScriptComponent copy;
        copy.Scripts.reserve(Scripts.size());
        for (const auto& script : Scripts)
        {
            copy.Scripts.push_back(script.ClonePersistent());
        }
        return copy;
    }

    static const char* GetStaticName() { return "ManagedScriptComponent"; }
};

CH_MARK_RFL(ManagedScriptComponent);

} // namespace CHEngine

#endif // CH_SCRIPTING_COMPONENTS_H
