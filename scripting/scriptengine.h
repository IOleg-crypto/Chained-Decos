#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "engine/core/engine_module.h"
#include "scriptengine_services.h"
#include <Coral/Assembly.hpp>
#include <string>
#include <unordered_map>


namespace Chained
{

class ScriptEngine : public EngineModule
{
public:
    ScriptEngine(bool enableScripting = true);
    virtual ~ScriptEngine() override;

    void Initialize() override;
    void Update(Timestep ts) override {}
    void Shutdown() override;


    bool LoadAppAssembly(const std::string& filepath);
    bool ReloadAssembly(const std::string& assemblyPath);
    bool RequestAssemblyReload(const std::string& assemblyPath, const char* requestSource);

    Coral::Type* GetScriptClass(const std::string& name);
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const { return m_Registry.GetScriptClasses(); }

    ScriptHost& GetHost() { return m_Host; }
    const ScriptHost& GetHost() const { return m_Host; }

    ScriptRegistry& GetRegistry() { return m_Registry; }
    const ScriptRegistry& GetRegistry() const { return m_Registry; }

    bool IsHostInitialized() const { return m_Host.IsInitialized(); }
    bool IsReloadInProgress() const { return m_Host.IsReloadInProgress(); }
    bool CanExecuteFrameScripts() const { return m_Host.IsInitialized() && !m_Host.IsReloadInProgress(); }

private:
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

private:
    ScriptHost m_Host;
    ScriptRegistry m_Registry;
    bool m_EnableScripting = true;
};

} // namespace Chained
#endif // CH_SCRIPT_ENGINE_H