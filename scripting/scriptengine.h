#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "scriptengine_services.h"
#include <Coral/Assembly.hpp>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Scene;

// Process-wide facade over the scripting host, type registry, and runtime scene handoff state.
// The singleton shape stays because CoreCLR and the editor/runtime layers expect one scripting service.
class ScriptEngine
{
public:
private:
    ScriptEngine();
    ~ScriptEngine();

public:

    static void Init();
    static void Shutdown();

    void InternalInit();
    void InternalShutdown();

    // Load (or reload) the game script DLL and refresh the type registry.
    bool LoadAppAssembly(const std::string& filepath);
    // Hot-reload: stops running scripts, unloads the old ALC, and loads the new DLL.
    bool ReloadAssembly();
    // UI-safe reload entry point with consistent guard/log behavior.
    bool RequestAssemblyReload(const char* requestSource);

    // Returns a pointer to the Coral::Type for the given short or full class name.
    // Search is case-insensitive. Returns nullptr if not found.
    Coral::Type* GetScriptClass(const std::string& name);

    // All discovered script types keyed by lowercase full name.
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const
    {
        return GetScriptRegistry().GetScriptClasses();
    }

    bool IsInitialized() const
    {
        return GetScriptHost().IsInitialized();
    }
    bool IsReloadInProgress() const
    {
        return GetScriptHost().IsReloadInProgress();
    }
    bool CanExecuteFrameScripts() const
    {
        return GetScriptHost().IsInitialized() && !GetScriptHost().IsReloadInProgress();
    }
public:
    static ScriptEngine& Get();
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H
