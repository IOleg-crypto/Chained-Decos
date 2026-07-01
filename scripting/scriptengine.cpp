#include "scriptengine.h"
#include "script_glue.h"
#include "engine/core/log.h"
#include "engine/foundation/engine_assert.h"
#include <exception>
#include <filesystem>

namespace Chained
{

ScriptEngine::ScriptEngine(bool enableScripting)
    : m_EnableScripting(enableScripting)
{
}

ScriptEngine::~ScriptEngine()
{
    if (m_Host.IsInitialized())
    {
        ScriptEngine::Shutdown();
    }
}

void ScriptEngine::Initialize()
{
    if (!m_EnableScripting)
    {
        CH_CORE_INFO("ScriptEngine: Scripting is disabled via config.");
        return;
    }

    if (m_Host.IsInitialized())
    {
        return;
    }

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR Host...");

    if (!m_Host.Init())
    {
        CH_CORE_ERROR("ScriptEngine: Failed to initialize ScriptHost.");
        return;
    }

    // Ініціалізація С++ / C# Glue прошарку
    ScriptGlue::Initialize();

    CH_CORE_INFO("ScriptEngine: CoreCLR host and Glue system initialized successfully.");
}

void ScriptEngine::Shutdown()
{
    if (!m_Host.IsInitialized())
    {
        return;
    }

    CH_CORE_INFO("ScriptEngine: Shutting down CoreCLR...");
    
    m_Registry.Clear();
    m_Host.Shutdown();
    
    CH_CORE_INFO("ScriptEngine: ScriptEngine cleanup complete.");
}

bool ScriptEngine::LoadAppAssembly(const std::string& path)
{
    if (path.empty())
    {
        return false;
    }

    if (!std::filesystem::exists(path))
    {
        CH_CORE_ERROR("ScriptEngine: Assembly file not found: {}", path);
        return false;
    }

    CH_CORE_INFO("ScriptEngine: Loading app assembly '{}'...", path);

    bool success = m_Host.LoadAppAssembly(path);
    if (success)
    {
        m_Registry.Clear();
        m_Registry.Discover(*m_Host.GetAppAssembly(), *m_Host.GetCoreAssembly());
    }
    return success;
}

bool ScriptEngine::ReloadAssembly(const std::string& assemblyPath)
{
    if (assemblyPath.empty())
    {
        CH_CORE_ERROR("ScriptEngine: Cannot reload assembly with an empty path.");
        return false;
    }

    if (!std::filesystem::exists(assemblyPath))
    {
        CH_CORE_ERROR("ScriptEngine: Reload failed. Assembly not found at {}", assemblyPath);
        return false;
    }

    CH_CORE_INFO("ScriptEngine: Reloading assembly '{}'...", assemblyPath);

    bool success = m_Host.ReloadAppAssembly(assemblyPath);
    if (success)
    {
        m_Registry.Clear();
        m_Registry.Discover(*m_Host.GetAppAssembly(), *m_Host.GetCoreAssembly());
    }
    return success;
}

bool ScriptEngine::RequestAssemblyReload(const std::string& assemblyPath, const char* requestSource)
{
    CH_CORE_INFO("ScriptEngine: Assembly reload requested by {}", requestSource);
    return ReloadAssembly(assemblyPath);
}

Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    return m_Registry.GetScriptClass(name);
}

} // namespace Chained