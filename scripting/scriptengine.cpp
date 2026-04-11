#include "scriptengine.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "scene_scripting.h"
#include "script_glue.h"
#include <exception>
#include <filesystem>

namespace CHEngine
{

namespace
{
static ScriptEngine* s_Instance = nullptr;
} // namespace

ScriptEngine::ScriptEngine()
{
    CH_CORE_ASSERT(!s_Instance, "ScriptEngine already exists!");
    s_Instance = this;
}

ScriptEngine::~ScriptEngine()
{
    InternalShutdown();
    s_Instance = nullptr;
}

ScriptEngine& ScriptEngine::Get()
{
    CH_CORE_ASSERT(s_Instance, "ScriptEngine not initialized!");
    return *s_Instance;
}

// ── Init / Shutdown ───────────────────────────────────────────────────────────
void ScriptEngine::Init()
{
    if (!s_Instance)
        s_Instance = new ScriptEngine();
    s_Instance->InternalInit();
}

void ScriptEngine::InternalInit()
{
    if (GetScriptAssemblyHost().IsInitialized())
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    if (!GetScriptAssemblyHost().Init())
        return;

    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");
}

void ScriptEngine::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        delete s_Instance;
        s_Instance = nullptr;
    }
}

void ScriptEngine::InternalShutdown()
{
    CH_CORE_INFO("ScriptEngine: Shutting down CoreCLR...");
    GetScriptTypeRegistry().Clear();
    GetScriptAssemblyHost().Shutdown();
    m_RuntimeSession.Reset();
}

// ── Assembly management ───────────────────────────────────────────────────────
bool ScriptEngine::LoadAppAssembly(const std::string& filepath)
{
    if (!GetScriptAssemblyHost().IsInitialized())
    {
        CH_CORE_WARN("ScriptEngine::LoadAppAssembly called before Init().");
        return false;
    }

    if (filepath.empty())
    {
        CH_CORE_WARN("ScriptEngine::LoadAppAssembly called with empty filepath.");
        return false;
    }

    if (!std::filesystem::exists(filepath))
    {
        CH_CORE_ERROR("ScriptEngine: Assembly not found at '{}'.", filepath);
        return false;
    }

    if (!GetScriptAssemblyHost().LoadAppAssembly(filepath))
    {
        GetScriptTypeRegistry().Clear();
        return false;
    }

    try
    {
        Coral::ManagedAssembly* coreAssembly = GetScriptAssemblyHost().GetCoreAssembly();
        Coral::ManagedAssembly* appAssembly = GetScriptAssemblyHost().GetAppAssembly();

        if (!coreAssembly || !appAssembly)
        {
            CH_CORE_ERROR("ScriptEngine: Assembly load completed but managed assemblies are missing.");
            GetScriptTypeRegistry().Clear();
            GetScriptAssemblyHost().ClearLoadedAssemblyState();
            return false;
        }

        ScriptGlue::RegisterInternalCalls(*coreAssembly);
        GetScriptTypeRegistry().Discover(*appAssembly, *coreAssembly);
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception during post-load setup for '{}': {}", filepath, e.what());
        GetScriptTypeRegistry().Clear();
        GetScriptAssemblyHost().ClearLoadedAssemblyState();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception during post-load setup for '{}'.", filepath);
        GetScriptTypeRegistry().Clear();
        GetScriptAssemblyHost().ClearLoadedAssemblyState();
        return false;
    }
}

bool ScriptEngine::ReloadAssembly()
{
    if (!GetScriptAssemblyHost().IsInitialized())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called before Init().");
        return false;
    }

    if (!m_RuntimeSession.BeginReload())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly skipped: reload already in progress.");
        return false;
    }

    struct ReloadScopeGuard
    {
        ScriptRuntimeSession& Session;
        ~ReloadScopeGuard()
        {
            Session.EndReload();
        }
    } guard{m_RuntimeSession};

    auto project = Project::GetActive();
    if (!project)
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly skipped: no active project.");
        return false;
    }

    auto& scripting = project->GetConfig().Scripting;
    if (scripting.ModuleName.empty())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly skipped: scripting module name is empty.");
        return false;
    }

    std::string dllName = scripting.ModuleName;
    if (dllName.find(".dll") == std::string::npos)
        dllName += ".dll";

    std::filesystem::path dllPath = scripting.ModuleDirectory / dllName;
    if (dllPath.is_relative())
        dllPath = Project::GetProjectDirectory() / dllPath;

    if (!std::filesystem::exists(dllPath))
    {
        CH_CORE_ERROR("ScriptEngine: Assembly not found at '{}'.", dllPath.string());
        return false;
    }

    // 1. Stop all running C# scripts cleanly (calls OnDestroy)
    if (m_RuntimeSession.GetActiveScene())
    {
        SceneScripting::Stop(m_RuntimeSession.GetActiveScene());
    }

    GetScriptTypeRegistry().Clear();
    if (!GetScriptAssemblyHost().ReloadAppAssembly(dllPath.string()))
    {
        CH_CORE_ERROR("ScriptEngine: Reload failed for '{}'.", dllPath.string());
        return false;
    }

    try
    {
        Coral::ManagedAssembly* coreAssembly = GetScriptAssemblyHost().GetCoreAssembly();
        Coral::ManagedAssembly* appAssembly = GetScriptAssemblyHost().GetAppAssembly();

        if (!coreAssembly || !appAssembly)
        {
            CH_CORE_ERROR("ScriptEngine: Reload completed but managed assemblies are missing.");
            GetScriptTypeRegistry().Clear();
            GetScriptAssemblyHost().ClearLoadedAssemblyState();
            return false;
        }

        ScriptGlue::RegisterInternalCalls(*coreAssembly);
        GetScriptTypeRegistry().Discover(*appAssembly, *coreAssembly);
        CH_CORE_INFO("ScriptEngine: Recreated ALC for reload.");
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception during reload setup for '{}': {}", dllPath.string(), e.what());
        GetScriptTypeRegistry().Clear();
        GetScriptAssemblyHost().ClearLoadedAssemblyState();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception during reload setup for '{}'.", dllPath.string());
        GetScriptTypeRegistry().Clear();
        GetScriptAssemblyHost().ClearLoadedAssemblyState();
        return false;
    }
}

bool ScriptEngine::RequestAssemblyReload(const char* requestSource)
{
    const char* source = (requestSource && requestSource[0] != '\0') ? requestSource : "ScriptEngine";

    if (IsReloadInProgress())
    {
        CH_CORE_INFO("{}: Script reload request ignored (reload already in progress).", source);
        return false;
    }

    if (!ReloadAssembly())
    {
        CH_CORE_WARN("{}: Script reload failed.", source);
        return false;
    }

    return true;
}

// ── Script lookup ─────────────────────────────────────────────────────────────
Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    return GetScriptTypeRegistry().GetScriptClass(name);
}

} // namespace CHEngine