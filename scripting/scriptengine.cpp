#include "scriptengine.h"
#include "script_glue.h"
#include "engine/core/log.h"
#include "engine/foundation/engine_assert.h"
#include <exception>
#include <filesystem>

namespace Chained
{
    ScriptEngine* ScriptEngine::s_Instance = nullptr;

    void ScriptEngine::Init(bool enableScripting)
    {
        CH_ASSERT(!s_Instance);
        s_Instance = new ScriptEngine(enableScripting);
        if (enableScripting)
        {
            s_Instance->OnInit();
        }
    }

    void ScriptEngine::Shutdown()
    {
        if (s_Instance)
        {
            s_Instance->OnShutdown();
            delete s_Instance;
            s_Instance = nullptr;
        }
    }

    ScriptEngine& ScriptEngine::Get()
    {
        CH_ASSERT(s_Instance);
        return *s_Instance;
    }

ScriptEngine::ScriptEngine(bool enableScripting)
    : m_EnableScripting(enableScripting)
{
}

ScriptEngine::~ScriptEngine() = default;

void ScriptEngine::OnInit()
{
    if (m_Host.IsInitialized())
    {
        return;
    }

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    if (!m_Host.Init())
    {
        return;
    }

    ScriptGlue::Initialize();

    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");
}

void ScriptEngine::OnShutdown()
{
    CH_CORE_INFO("ScriptEngine: Shutting down CoreCLR...");
    m_Host.Shutdown();
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