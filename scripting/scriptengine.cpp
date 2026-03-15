#include "scriptengine.h"
#include "engine/core/log.h"
#include "engine/scene/scene.h"
#include "engine/scene/project.h"
#include "scene_scripting.h"
#include "engine/core/application.h"
#include <Coral/ManagedObject.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include "script_glue.h"
#include "engine/core/base.h"

#ifdef CH_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace CHEngine {

// ── Lifecycle ────────────────────────────────────────────────────────────────
static ScriptEngine* s_Instance = nullptr;

ScriptEngine::ScriptEngine()
    : m_ActiveScene(nullptr), m_IsInitialized(false)
{
    CH_CORE_ASSERT(!s_Instance, "ScriptEngine already exists!");
    s_Instance = this;
}

ScriptEngine::~ScriptEngine()
{
    if (m_IsInitialized)
    {
        Shutdown();
    }
    s_Instance = nullptr;
}

ScriptEngine& ScriptEngine::Get()
{
    CH_CORE_ASSERT(s_Instance, "ScriptEngine not initialized!");
    return *s_Instance;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::string ToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static std::filesystem::path GetExecutableDir()
{
#ifdef CH_PLATFORM_WINDOWS
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif defined(CH_PLATFORM_LINUX)
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    if (count != -1)
        return std::filesystem::path(std::string(path, count)).parent_path();
#endif
    return std::filesystem::current_path();
}

// Shadow-copy directory for DLLs — avoids file locks from CoreCLR
static std::filesystem::path GetShadowDir()
{
    return std::filesystem::temp_directory_path() / "CHEngine_shadow";
}

/// Copy a DLL (and optional .pdb) to a uniquely-named shadow file.
/// Returns the path to the shadow copy.
static std::filesystem::path ShadowCopyDll(const std::filesystem::path& original)
{
    auto shadowDir = GetShadowDir();
    std::filesystem::create_directories(shadowDir);

    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string shadowName = original.stem().string() + "_" + std::to_string(ts) + ".dll";
    std::filesystem::path shadowDll = shadowDir / shadowName;

    std::error_code ec;
    std::filesystem::copy_file(original, shadowDll,
                               std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to shadow-copy '{}' -> '{}': {}",
                      original.string(), shadowDll.string(), ec.message());
        return ""; // Return empty to indicate failure
    }

    // Copy .pdb for debugger support
    auto pdbOrig = original; pdbOrig.replace_extension(".pdb");
    if (std::filesystem::exists(pdbOrig))
    {
        auto pdbShadow = shadowDll; pdbShadow.replace_extension(".pdb");
        std::filesystem::copy_file(pdbOrig, pdbShadow,
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }

    CH_CORE_INFO("ScriptEngine: Shadow-copied '{}' -> '{}'", original.string(), shadowDll.string());
    return shadowDll;
}

/// Remove all old shadow copies (called before creating a new one)
static void CleanupShadowCopies()
{
    auto shadowDir = GetShadowDir();
    std::error_code ec;
    if (std::filesystem::exists(shadowDir, ec))
    {
        std::filesystem::remove_all(shadowDir, ec);
        if (ec)
            CH_CORE_WARN("ScriptEngine: Could not clean shadow dir: {}", ec.message());
    }
}

// ── Init / Shutdown ───────────────────────────────────────────────────────────
void ScriptEngine::Init()
{
    if (m_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    Coral::HostSettings settings;
    // Coral looks for Coral.Managed.dll in this directory.
    settings.CoralDirectory = GetExecutableDir().string();

    auto status = m_Host.Initialize(settings);
    if (status != Coral::CoralInitStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to initialize Coral! Status: {}", (int)status);
        return;
    }

    m_IsInitialized = true;
    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");

    // Create the initial AssemblyLoadContext
    m_AppAssemblyContext = m_Host.CreateAssemblyLoadContext("GameScriptsALC");
}

void ScriptEngine::Shutdown()
{
    if (!m_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Shutting down...");
    m_ScriptClasses.clear();
    m_AppAssembly = nullptr;
    m_CoreAssembly = nullptr;
    m_Host.Shutdown();
    CleanupShadowCopies();
    m_IsInitialized = false;
}

// ── Assembly management ───────────────────────────────────────────────────────
void ScriptEngine::LoadAppAssembly(const std::string& filepath)
{
    if (!m_IsInitialized)
    {
        CH_CORE_WARN("ScriptEngine::LoadAppAssembly called before Init().");
        return;
    }

    try
    {
        // 1. Ensure our Core Managed library is loaded first (in the same ALC)
        // This provides the base CHEngine.Script class for discovery.
        std::filesystem::path corePath = GetExecutableDir() / "CHEngine.Managed.dll";
        if (!std::filesystem::exists(corePath)) {
            corePath = "CHEngine.Managed.dll"; 
        }

        // Shadow-copy to avoid file locks from CoreCLR
        auto shadowCore = ShadowCopyDll(corePath);
        if (shadowCore.empty())
        {
            CH_CORE_ERROR("ScriptEngine: Failed to shadow-copy core assembly '{}'", corePath.string());
            return;
        }

        m_CoreAssembly = &m_AppAssemblyContext.LoadAssembly(shadowCore.string());
        if (!m_CoreAssembly || m_CoreAssembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to load core assembly '{}'. Status: {}", 
                          corePath.string(), m_CoreAssembly ? (int)m_CoreAssembly->GetLoadStatus() : -1);
            m_CoreAssembly = nullptr;
            return;
        }

        // 2. Load the actual game scripts (also shadow-copied)
        auto shadowApp = ShadowCopyDll(std::filesystem::path(filepath));
        if (shadowApp.empty())
        {
            CH_CORE_ERROR("ScriptEngine: Failed to shadow-copy app assembly '{}'", filepath);
            return;
        }

        m_AppAssembly = &m_AppAssemblyContext.LoadAssembly(shadowApp.string());

        if (m_AppAssembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to load assembly '{}'. Status: {}",
                          filepath, (int)m_AppAssembly->GetLoadStatus());
            m_AppAssembly = nullptr;
            return;
        }

        CH_CORE_INFO("ScriptEngine: Loaded assembly '{}'.", filepath);
        
        if (m_CoreAssembly)
            ScriptGlue::RegisterInternalCalls(*m_CoreAssembly);
        DiscoverScriptTypes();
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception loading assembly '{}': {}", filepath, e.what());
        m_AppAssembly = nullptr;
    }
}

void ScriptEngine::ReloadAssembly()
{
    if (!m_IsInitialized)
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
    if (m_ActiveScene)
        SceneScripting::Stop(m_ActiveScene);

    // 2. Unload the old AssemblyLoadContext so the DLL file is released
    m_ScriptClasses.clear();
    m_AppAssembly = nullptr;
    m_CoreAssembly = nullptr;
    m_Host.UnloadAssemblyLoadContext(m_AppAssemblyContext);
    CH_CORE_INFO("ScriptEngine: Old ALC unloaded.");

    // 3. Clean old shadow copies so we don't accumulate temp files
    CleanupShadowCopies();

    // 4. Fresh ALC + load the new DLL
    m_AppAssemblyContext = m_Host.CreateAssemblyLoadContext("GameScriptsALC");
    LoadAppAssembly(dllPath.string());
}

// ── Type discovery ─────────────────────────────────────────────────────────────
void ScriptEngine::DiscoverScriptTypes()
{
    if (!m_AppAssembly)
        return;

    m_ScriptClasses.clear();

    // The base script type lives in CHEngine.Managed.dll.
    // We must find it in the Core assembly, not the App assembly.
    Coral::Type scriptBaseType = m_CoreAssembly->GetLocalType("CHEngine.Script");

    if (!scriptBaseType)
    {
        CH_CORE_ERROR("ScriptEngine: Could not find base class 'CHEngine.Script' in Core assembly!");
        return;
    }

    auto types = m_AppAssembly->GetTypes();
    for (auto& type : types)
    {
        // Skip the base class itself; only register concrete subclasses
        if (!type->IsSubclassOf(scriptBaseType) || *type == scriptBaseType)
            continue;

        std::string fullName = (std::string)type->GetFullName();
        std::string key      = ToLower(fullName);

        m_ScriptClasses[key] = *type;
        CH_CORE_INFO("ScriptEngine: Registered script '{}' (key: '{}')", fullName, key);
    }

    CH_CORE_INFO("ScriptEngine: {} script(s) registered.", m_ScriptClasses.size());
}

// ── Script lookup ─────────────────────────────────────────────────────────────
Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    std::string key = ToLower(name);

    // Exact full-name match (most common)
    auto it = m_ScriptClasses.find(key);
    if (it != m_ScriptClasses.end())
        return &it->second;

    // Fallback: partial suffix match — allows using bare "PlayerController"
    // when the stored key is "chaineddecos.scripts.playercontroller"
    for (auto& [storedKey, type] : m_ScriptClasses)
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