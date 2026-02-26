#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H

#include <string>
#include <vector>

namespace CHEngine
{

// Represents a single C# script instance attached to an entity.
struct ManagedScriptInstance
{
    std::string ClassName;    // Full qualified name, e.g. "ChainedDecos.Scripts.PlayerController"
    void*       Instance     = nullptr; // Runtime only: Coral::ManagedObject*
    bool        NeedsStart   = false;   // True until OnStart() has been invoked

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
