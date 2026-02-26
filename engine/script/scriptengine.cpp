#include "scriptengine.h"
#include "engine/core/log.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_scripting.h"
#include <Coral/ManagedObject.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include "engine/script/script_glue.h"

namespace CHEngine {

// ── Static member definitions ────────────────────────────────────────────────
Scene*                                     ScriptEngine::s_ActiveScene          = nullptr;
Coral::HostInstance                        ScriptEngine::s_Host;
Coral::AssemblyLoadContext                 ScriptEngine::s_AppAssemblyContext;
Coral::ManagedAssembly*                    ScriptEngine::s_AppAssembly          = nullptr;
Coral::ManagedAssembly*                    ScriptEngine::s_CoreAssembly         = nullptr;
std::unordered_map<std::string, Coral::Type> ScriptEngine::s_ScriptClasses;
bool                                       ScriptEngine::s_IsInitialized        = false;

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ── Init / Shutdown ───────────────────────────────────────────────────────────
void ScriptEngine::Init()
{
    if (s_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    Coral::HostSettings settings;
    // Coral looks for Coral.Managed.dll in this directory.
    // It must be next to the engine executable.
    settings.CoralDirectory = ".";

    auto status = s_Host.Initialize(settings);
    if (status != Coral::CoralInitStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to initialize Coral! Status: {}", (int)status);
        return;
    }

    s_IsInitialized = true;
    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");

    // Create the initial AssemblyLoadContext
    s_AppAssemblyContext = s_Host.CreateAssemblyLoadContext("GameScriptsALC");
}

void ScriptEngine::Shutdown()
{
    if (!s_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Shutting down...");
    s_ScriptClasses.clear();
    s_AppAssembly = nullptr;
    s_CoreAssembly = nullptr;
    s_Host.Shutdown();
    s_IsInitialized = false;
}

// ── Assembly management ───────────────────────────────────────────────────────
void ScriptEngine::LoadAppAssembly(const std::string& filepath)
{
    if (!s_IsInitialized)
    {
        CH_CORE_WARN("ScriptEngine::LoadAppAssembly called before Init().");
        return;
    }

    try
    {
        // 1. Ensure our Core Managed library is loaded first (in the same ALC)
        // This provides the base CHEngine.Script class for discovery.
        // We assume CHEngine.Managed.dll is in the same directory as the executable.
        std::filesystem::path corePath = std::filesystem::current_path() / "CHEngine.Managed.dll";
        if (!std::filesystem::exists(corePath)) {
            // Fallback: look in the same directory as the current application assembly if relative?
            // Actually, it should be next to CHEngine.exe
            corePath = "CHEngine.Managed.dll"; 
        }

        s_CoreAssembly = &s_AppAssemblyContext.LoadAssembly(corePath.string());

        // 2. Load the actual game scripts
        s_AppAssembly = &s_AppAssemblyContext.LoadAssembly(filepath);

        if (s_AppAssembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to load assembly '{}'. Status: {}",
                          filepath, (int)s_AppAssembly->GetLoadStatus());
            s_AppAssembly = nullptr;
            return;
        }

        CH_CORE_INFO("ScriptEngine: Loaded assembly '{}'.", filepath);
        
        if (s_CoreAssembly)
            ScriptGlue::RegisterInternalCalls(*s_CoreAssembly);
        DiscoverScriptTypes();
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception loading assembly '{}': {}", filepath, e.what());
        s_AppAssembly = nullptr;
    }
}

void ScriptEngine::ReloadAssembly()
{
    if (!s_IsInitialized)
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called before Init().");
        return;
    }

    auto project = Project::GetActive();
    if (!project)
        return;

    auto& scripting = project->GetConfig().Scripting;
    if (scripting.ModuleName.empty())
        return;

    std::string dllName = scripting.ModuleName;
    if (dllName.find(".dll") == std::string::npos)
        dllName += ".dll";

    std::filesystem::path dllPath = scripting.ModuleDirectory / dllName;
    if (dllPath.is_relative())
        dllPath = Project::GetProjectDirectory() / dllPath;

    if (!std::filesystem::exists(dllPath))
    {
        CH_CORE_ERROR("ScriptEngine: Assembly not found at '{}'.", dllPath.string());
        return;
    }

    // 1. Stop all running C# scripts cleanly (calls OnDestroy)
    if (s_ActiveScene)
        SceneScripting::Stop(s_ActiveScene);

    // 2. Unload the old AssemblyLoadContext so the DLL file is released
    s_ScriptClasses.clear();
    s_AppAssembly = nullptr;
    s_CoreAssembly = nullptr;
    s_Host.UnloadAssemblyLoadContext(s_AppAssemblyContext);
    CH_CORE_INFO("ScriptEngine: Old ALC unloaded.");

    // 3. Fresh ALC + load the new DLL
    s_AppAssemblyContext = s_Host.CreateAssemblyLoadContext("GameScriptsALC");
    LoadAppAssembly(dllPath.string());
}

// ── Type discovery ─────────────────────────────────────────────────────────────
void ScriptEngine::DiscoverScriptTypes()
{
    if (!s_AppAssembly)
        return;

    s_ScriptClasses.clear();

    // The base script type lives in CHEngine.Managed.dll.
    // We must find it in the Core assembly, not the App assembly.
    Coral::Type scriptBaseType = s_CoreAssembly->GetLocalType("CHEngine.Script");

    if (!scriptBaseType)
    {
        CH_CORE_ERROR("ScriptEngine: Could not find base class 'CHEngine.Script' in Core assembly!");
        return;
    }

    auto types = s_AppAssembly->GetTypes();
    for (auto& type : types)
    {
        // Skip the base class itself; only register concrete subclasses
        if (!type->IsSubclassOf(scriptBaseType) || *type == scriptBaseType)
            continue;

        std::string fullName = (std::string)type->GetFullName();
        std::string key      = ToLower(fullName);

        s_ScriptClasses[key] = *type;
        CH_CORE_INFO("ScriptEngine: Registered script '{}' (key: '{}')", fullName, key);
    }

    CH_CORE_INFO("ScriptEngine: {} script(s) registered.", s_ScriptClasses.size());
}

// ── Script lookup ─────────────────────────────────────────────────────────────
Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    std::string key = ToLower(name);

    // Exact full-name match (most common)
    auto it = s_ScriptClasses.find(key);
    if (it != s_ScriptClasses.end())
        return &it->second;

    // Fallback: partial suffix match — allows using bare "PlayerController"
    // when the stored key is "chaineddecos.scripts.playercontroller"
    for (auto& [storedKey, type] : s_ScriptClasses)
    {
        // storedKey ends with ".<key>" or equals <key>
        if (storedKey == key)
            return &const_cast<Coral::Type&>(type);

        if (storedKey.size() >= key.size() + 1)
        {
            auto suffix = storedKey.substr(storedKey.size() - key.size());
            auto dot    = storedKey[storedKey.size() - key.size() - 1];
            if (dot == '.' && suffix == key)
                return &const_cast<Coral::Type&>(type);
        }
    }

    CH_CORE_WARN("ScriptEngine: No script found for name '{}' (key: '{}')", name, key);
    return nullptr;
}

} // namespace CHEngine