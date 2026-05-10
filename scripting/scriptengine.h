#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "scriptengine_services.h"
#include "engine/core/engine_service.h"
#include <Coral/Assembly.hpp>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Scene;

// Engine service that owns the scripting host, type registry, and runtime state.
// Lifecycle is controlled by the EngineService pattern (OnInit -> OnShutdown).
class ScriptEngine : public EngineService
{
public:
    explicit ScriptEngine(bool enableScripting = true);
    ~ScriptEngine() override;

    // Prevent copying to maintain ownership semantics
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    void Init();
    void Deinit();

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

protected:
    void OnInit() override;
    void OnShutdown() override;

private:
    ScriptHost m_Host;
    ScriptRegistry m_Registry;
    bool m_EnableScripting = true;
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H
