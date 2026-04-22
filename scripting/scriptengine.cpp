#include "scriptengine.h"
#include "engine/core/service_locator.h"
#include "engine/core/ch_assert.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "script_glue.h"
#include "engine/scene/SceneScriptingManager.h"
#include <exception>
#include <filesystem>

namespace CHEngine
{

ScriptEngine& ScriptEngine::Get()
{
    return ServiceLocator::Get<ScriptEngine>();
}

bool ScriptEngine::IsInitializedGlobal()
{
    return ServiceLocator::Has<ScriptEngine>();
}

ScriptEngine::ScriptEngine()
{
}

ScriptEngine::~ScriptEngine()
{
}

// ── Initialize / Shutdown ──────────────────────────────────────────────────
void ScriptEngine::Initialize()
{
    InternalInit();
}

void ScriptEngine::Shutdown()
{
    InternalShutdown();
}

void ScriptEngine::InternalInit()
{
    if (GetScriptHost().IsInitialized())
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    if (!GetScriptHost().Init())
        return;

    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");
}


void ScriptEngine::InternalShutdown()
{
    CH_CORE_INFO("ScriptEngine: Shutting down CoreCLR...");
    GetScriptRegistry().Clear();
    GetScriptHost().Shutdown();
}

// ── Assembly management ───────────────────────────────────────────────────────
bool ScriptEngine::LoadAppAssembly(const std::string& filepath)
{
    if (!GetScriptHost().IsInitialized())
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

    if (!GetScriptHost().LoadAppAssembly(filepath))
    {
        GetScriptRegistry().Clear();
        return false;
    }

    try
    {
        Coral::ManagedAssembly* coreAssembly = GetScriptHost().GetCoreAssembly();
        Coral::ManagedAssembly* appAssembly = GetScriptHost().GetAppAssembly();

        if (!coreAssembly || !appAssembly)
        {
            CH_CORE_ERROR("ScriptEngine: Assembly load completed but managed assemblies are missing.");
            GetScriptRegistry().Clear();
            GetScriptHost().ClearLoadedAssemblyState();
            return false;
        }

        ScriptGlue::RegisterInternalCalls(*coreAssembly);
        GetScriptRegistry().Discover(*appAssembly, *coreAssembly);
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception during post-load setup for '{}': {}", filepath, e.what());
        GetScriptRegistry().Clear();
        GetScriptHost().ClearLoadedAssemblyState();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception during post-load setup for '{}'.", filepath);
        GetScriptRegistry().Clear();
        GetScriptHost().ClearLoadedAssemblyState();
        return false;
    }
}

bool ScriptEngine::ReloadAssembly()
{
    if (!GetScriptHost().IsInitialized())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called before Init().");
        return false;
    }

    if (GetScriptHost().IsReloadInProgress())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly skipped: reload already in progress.");
        return false;
    }

    GetScriptHost().SetReloadInProgress(true);
    
    struct ReloadScopeGuard
    {
        ~ReloadScopeGuard()
        {
            GetScriptHost().SetReloadInProgress(false);
        }
    } guard;

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

    // Stop all active scripts across all scenes before unloading assemblies.
    SceneScriptingManager::ResetAll();

    GetScriptRegistry().Clear();
    if (!GetScriptHost().ReloadAppAssembly(dllPath.string()))
    {
        CH_CORE_ERROR("ScriptEngine: Reload failed for '{}'.", dllPath.string());
        return false;
    }

    try
    {
        Coral::ManagedAssembly* coreAssembly = GetScriptHost().GetCoreAssembly();
        Coral::ManagedAssembly* appAssembly = GetScriptHost().GetAppAssembly();

        if (!coreAssembly || !appAssembly)
        {
            CH_CORE_ERROR("ScriptEngine: Reload completed but managed assemblies are missing.");
            GetScriptRegistry().Clear();
            GetScriptHost().ClearLoadedAssemblyState();
            return false;
        }

        ScriptGlue::RegisterInternalCalls(*coreAssembly);
        GetScriptRegistry().Discover(*appAssembly, *coreAssembly);
        CH_CORE_INFO("ScriptEngine: Recreated ALC for reload.");
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception during reload setup for '{}': {}", dllPath.string(), e.what());
        GetScriptRegistry().Clear();
        GetScriptHost().ClearLoadedAssemblyState();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception during reload setup for '{}'.", dllPath.string());
        GetScriptRegistry().Clear();
        GetScriptHost().ClearLoadedAssemblyState();
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
    return GetScriptRegistry().GetScriptClass(name);
}

} // namespace CHEngine