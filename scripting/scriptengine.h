#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "scriptengine_services.h"
#include <Coral/Assembly.hpp>
#include <string>
#include <unordered_map>

namespace Chained
{

class Scene;

class ScriptEngine
{
public:
    static void Init(bool enableScripting = true);
    static void Shutdown();
    static ScriptEngine& Get();

    // Load (or reload) the game script DLL and refresh the type registry.
    bool LoadAppAssembly(const std::string& filepath);
    // Hot-reload: stops running scripts, unloads the old ALC, and loads the new DLL from the specified path.
    bool ReloadAssembly(const std::string& assemblyPath);
    // UI-safe reload entry point with consistent guard/log behavior.
    bool RequestAssemblyReload(const std::string& assemblyPath, const char* requestSource);

    Coral::Type* GetScriptClass(const std::string& name);
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const { return m_Registry.GetScriptClasses(); }

    ScriptHost& GetHost() { return m_Host; }
    const ScriptHost& GetHost() const { return m_Host; }

    ScriptRegistry& GetRegistry() { return m_Registry; }
    const ScriptRegistry& GetRegistry() const { return m_Registry; }

    bool IsHostInitialized() const
    {
        return m_Host.IsInitialized();
    }
    bool IsReloadInProgress() const
    {
        return m_Host.IsReloadInProgress();
    }
    bool CanExecuteFrameScripts() const
    {
        return m_Host.IsInitialized() && !m_Host.IsReloadInProgress();
    }

private:
    explicit ScriptEngine(bool enableScripting = true);
    virtual ~ScriptEngine();

    // Prevent copying to maintain ownership semantics
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    void OnInit();
    void OnShutdown();

protected:
    ScriptHost m_Host;
    ScriptRegistry m_Registry;
    bool m_EnableScripting = true;

    static ScriptEngine* s_Instance;
};

} // namespace Chained
#endif // CH_SCRIPT_ENGINE_H
