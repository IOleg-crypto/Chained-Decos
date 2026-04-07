#include "scriptengine.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "scene_scripting.h"
#include "engine/core/filesystem_utils.h"
#include <Coral/ManagedObject.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <vector>
#include "script_glue.h"

namespace CHEngine {

namespace
{
constexpr const char* kGameScriptsAlcName = "GameScriptsALC";

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string GetShortTypeName(const std::string& fullName)
{
    const size_t lastDot = fullName.find_last_of('.');
    if (lastDot == std::string::npos || lastDot + 1 >= fullName.size())
        return fullName;

    return fullName.substr(lastDot + 1);
}

void AppendBuildBinCandidates(std::vector<std::filesystem::path>& out, const std::filesystem::path& root)
{
    if (root.empty())
        return;

    static constexpr std::array<const char*, 4> kPresetDirs = {
        "windows-ninja",
        "windows-vs2022",
        "linux-gcc",
        "linux-clang",
    };

    for (const char* preset : kPresetDirs)
    {
        out.push_back(root / "build" / preset / "bin");
    }
}

std::filesystem::path ResolveCoralDirectory()
{
    std::vector<std::filesystem::path> candidateDirs;
    candidateDirs.push_back(FilesystemUtils::GetExecutableDirectory());
    candidateDirs.push_back(std::filesystem::current_path());

#ifdef PROJECT_ROOT_DIR
    const std::filesystem::path engineRoot = std::filesystem::path(PROJECT_ROOT_DIR);
    candidateDirs.push_back(engineRoot);
    AppendBuildBinCandidates(candidateDirs, engineRoot);
#endif

    if (auto project = Project::GetActive())
    {
        const std::filesystem::path projectDir = project->GetConfig().ProjectDirectory;
        candidateDirs.push_back(projectDir);
        AppendBuildBinCandidates(candidateDirs, projectDir);
    }

    std::vector<std::string> checkedPaths;
    for (const auto& candidateRaw : candidateDirs)
    {
        if (candidateRaw.empty())
            continue;

        std::error_code ec;
        const std::filesystem::path candidate = std::filesystem::absolute(candidateRaw, ec).lexically_normal();
        const std::filesystem::path managedPath = ec ? (candidateRaw / "Coral.Managed.dll")
                                                     : (candidate / "Coral.Managed.dll");
        checkedPaths.push_back(managedPath.string());

        if (std::filesystem::exists(managedPath))
            return ec ? candidateRaw : candidate;
    }

    const std::filesystem::path fallback = FilesystemUtils::GetExecutableDirectory();
    CH_CORE_WARN("ScriptEngine: Coral.Managed.dll not found in fallback candidates. Using executable directory: '{}'.",
                 fallback.string());
    for (const auto& checked : checkedPaths)
    {
        CH_CORE_TRACE("ScriptEngine: Checked '{}'", checked);
    }
    return fallback;
}

std::filesystem::path ResolveCoreAssemblyPath(const std::filesystem::path& coralDir)
{
    std::vector<std::filesystem::path> coreCandidates;
    if (!coralDir.empty())
        coreCandidates.push_back(coralDir / "CHEngine.Managed.dll");

    const std::filesystem::path exeDir = FilesystemUtils::GetExecutableDirectory();
    coreCandidates.push_back(exeDir / "CHEngine.Managed.dll");
    coreCandidates.push_back(std::filesystem::current_path() / "CHEngine.Managed.dll");

#ifdef PROJECT_ROOT_DIR
    AppendBuildBinCandidates(coreCandidates, std::filesystem::path(PROJECT_ROOT_DIR));
#endif

    if (auto project = Project::GetActive())
    {
        AppendBuildBinCandidates(coreCandidates, project->GetConfig().ProjectDirectory);
    }

    for (const auto& candidate : coreCandidates)
    {
        if (!candidate.empty() && std::filesystem::exists(candidate))
            return candidate;
    }

    return std::filesystem::path("CHEngine.Managed.dll");
}
} // namespace

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
    InternalShutdown();
    s_Instance = nullptr;
}

ScriptEngine& ScriptEngine::Get()
{
    CH_CORE_ASSERT(s_Instance, "ScriptEngine not initialized!");
    return *s_Instance;
}

void ScriptEngine::ClearLoadedAssemblyState()
{
    m_ScriptClasses.clear();
    m_ShortNameToFullName.clear();
    m_AppAssembly = nullptr;
    m_CoreAssembly = nullptr;
}

bool ScriptEngine::RecreateAssemblyLoadContext(bool unloadCurrent)
{
    if (!m_IsInitialized)
    {
        CH_CORE_ERROR("ScriptEngine: Cannot recreate ALC before host initialization.");
        return false;
    }

    if (unloadCurrent)
    {
        try
        {
            m_Host.UnloadAssemblyLoadContext(m_AppAssemblyContext);
        }
        catch (const std::exception& e)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to unload current ALC: {}", e.what());
            return false;
        }
        catch (...)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to unload current ALC (unknown exception).");
            return false;
        }
    }

    try
    {
        m_AppAssemblyContext = m_Host.CreateAssemblyLoadContext(kGameScriptsAlcName);
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to create ALC '{}': {}", kGameScriptsAlcName, e.what());
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to create ALC '{}' (unknown exception).", kGameScriptsAlcName);
        return false;
    }
}

bool ScriptEngine::LoadAssembliesTransactional(const std::filesystem::path& appAssemblyPath)
{
    Coral::ManagedAssembly* loadedCore = nullptr;
    Coral::ManagedAssembly* loadedApp = nullptr;

    auto rollback = [&]() {
        ClearLoadedAssemblyState();
        if (!RecreateAssemblyLoadContext(true))
        {
            CH_CORE_ERROR("ScriptEngine: Rollback failed to recreate ALC. Scripting remains unavailable.");
        }
    };

    const std::filesystem::path corePath = ResolveCoreAssemblyPath(m_CoralDirectory);

    try
    {
        loadedCore = &m_AppAssemblyContext.LoadAssembly(corePath.string());
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception loading core assembly '{}': {}", corePath.string(), e.what());
        rollback();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception loading core assembly '{}'.", corePath.string());
        rollback();
        return false;
    }

    if (!loadedCore || loadedCore->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to load core assembly '{}'. Status: {}", corePath.string(),
                      loadedCore ? (int)loadedCore->GetLoadStatus() : -1);
        rollback();
        return false;
    }

    try
    {
        loadedApp = &m_AppAssemblyContext.LoadAssembly(appAssemblyPath.string());
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception loading app assembly '{}': {}", appAssemblyPath.string(), e.what());
        rollback();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception loading app assembly '{}'.", appAssemblyPath.string());
        rollback();
        return false;
    }

    if (!loadedApp || loadedApp->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to load app assembly '{}'. Status: {}", appAssemblyPath.string(),
                      loadedApp ? (int)loadedApp->GetLoadStatus() : -1);
        rollback();
        return false;
    }

    m_CoreAssembly = loadedCore;
    m_AppAssembly = loadedApp;

    CH_CORE_INFO("ScriptEngine: Loaded core assembly '{}'.", corePath.string());
    CH_CORE_INFO("ScriptEngine: Loaded app assembly '{}'.", appAssemblyPath.string());
    return true;
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
    if (m_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    Coral::HostSettings settings;

    m_CoralDirectory = ResolveCoralDirectory();
    settings.CoralDirectory = m_CoralDirectory.string();
    CH_CORE_INFO("ScriptEngine: Using Coral directory '{}'.", m_CoralDirectory.string());

    auto status = m_Host.Initialize(settings);
    if (status != Coral::CoralInitStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to initialize Coral! Status: {}", (int)status);
        return;
    }

    m_IsInitialized = true;
    CH_CORE_INFO("ScriptEngine: CoreCLR initialized.");

    // Create the initial AssemblyLoadContext
    if (!RecreateAssemblyLoadContext(false))
    {
        m_Host.Shutdown();
        m_IsInitialized = false;
    }
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
    if (!m_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Shutting down CoreCLR...");
    ClearLoadedAssemblyState();
    m_ActiveScene = nullptr;
    m_PendingScenePath.clear();
    m_ReloadInProgress = false;
    m_CoralDirectory.clear();
    m_Host.Shutdown();
    m_IsInitialized = false;
}

// ── Assembly management ───────────────────────────────────────────────────────
bool ScriptEngine::LoadAppAssembly(const std::string& filepath)
{
    if (!m_IsInitialized)
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

    if (!LoadAssembliesTransactional(std::filesystem::path(filepath)))
    {
        return false;
    }

    try
    {
        ScriptGlue::RegisterInternalCalls(*m_CoreAssembly);
        DiscoverScriptTypes();
        return true;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("ScriptEngine: Exception during post-load setup for '{}': {}", filepath, e.what());
        ClearLoadedAssemblyState();
        return false;
    }
    catch (...)
    {
        CH_CORE_ERROR("ScriptEngine: Unknown exception during post-load setup for '{}'.", filepath);
        ClearLoadedAssemblyState();
        return false;
    }
}

bool ScriptEngine::ReloadAssembly()
{
    if (!m_IsInitialized)
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called before Init().");
        return false;
    }

    if (m_ReloadInProgress)
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly skipped: reload already in progress.");
        return false;
    }

    m_ReloadInProgress = true;
    struct ReloadScopeGuard
    {
        bool& Flag;
        ~ReloadScopeGuard()
        {
            Flag = false;
        }
    } guard{m_ReloadInProgress};

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
    if (m_ActiveScene)
    {
        SceneScripting::Stop(m_ActiveScene);
    }

    // 2. Unload the old AssemblyLoadContext so the DLL file is released
    ClearLoadedAssemblyState();
    if (!RecreateAssemblyLoadContext(true))
    {
        return false;
    }
    CH_CORE_INFO("ScriptEngine: Recreated ALC for reload.");

    // 3. Fresh ALC + load the new DLL directly from the project
    const bool loaded = LoadAppAssembly(dllPath.string());
    if (!loaded)
    {
        CH_CORE_ERROR("ScriptEngine: Reload failed for '{}'.", dllPath.string());
    }
    return loaded;
}

// ── Type discovery ─────────────────────────────────────────────────────────────
void ScriptEngine::DiscoverScriptTypes()
{
    if (!m_AppAssembly)
        return;

    if (!m_CoreAssembly)
    {
        CH_CORE_ERROR("ScriptEngine: Core assembly is not loaded. Type discovery aborted.");
        return;
    }

    m_ScriptClasses.clear();
    m_ShortNameToFullName.clear();

    // The base script type lives in CHEngine.Managed.dll.
    // We must find it in the Core assembly, not the App assembly.
    Coral::Type scriptBaseType = m_CoreAssembly->GetLocalType("CHEngine.Script");

    if (!scriptBaseType)
    {
        CH_CORE_ERROR("ScriptEngine: Could not find base class 'CHEngine.Script' in Core assembly! Type discovery aborted.");
        return;
    }
    CH_CORE_INFO("ScriptEngine: Found base script type '{}'.", (std::string)scriptBaseType.GetFullName());

    auto types = m_AppAssembly->GetTypes();
    CH_CORE_INFO("ScriptEngine: Discovery - searching {} types in assembly '{}'...", types.size(), (std::string)m_AppAssembly->GetName());

    for (auto& type : types)
    {
        if (*type == scriptBaseType) continue;

        if (!type->IsSubclassOf(scriptBaseType))
        {
            // Optional: log non-script types if we suspect inheritance bugs
            // CH_CORE_TRACE("ScriptEngine: Skipping type '{}' (not a subclass of Script)", (std::string)type->GetFullName());
            continue;
        }

        std::string fullName = (std::string)type->GetFullName();
        std::string key = ToLowerCopy(fullName);

        m_ScriptClasses[key] = *type;

        std::string shortKey = ToLowerCopy(GetShortTypeName(fullName));
        auto [shortIt, inserted] = m_ShortNameToFullName.emplace(shortKey, key);
        if (!inserted && shortIt->second != key)
        {
            // Empty marker means ambiguous short name; full name is required.
            shortIt->second.clear();
        }

        CH_CORE_TRACE("ScriptEngine: Registered script '{}' (key: '{}')", fullName, key);
    }

    CH_CORE_INFO("ScriptEngine: {} script(s) registered ({} short-name keys).", m_ScriptClasses.size(),
                 m_ShortNameToFullName.size());
}

// ── Script lookup ─────────────────────────────────────────────────────────────
Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    std::string key = ToLowerCopy(name);

    // Exact full-name match (most common)
    auto it = m_ScriptClasses.find(key);
    if (it != m_ScriptClasses.end())
        return &it->second;

    // Fast short-name match (for simple class names).
    auto shortIt = m_ShortNameToFullName.find(key);
    if (shortIt != m_ShortNameToFullName.end())
    {
        if (shortIt->second.empty())
        {
            CH_CORE_WARN("ScriptEngine: Ambiguous short script name '{}'. Use fully-qualified name.", name);
            return nullptr;
        }

        auto fullIt = m_ScriptClasses.find(shortIt->second);
        if (fullIt != m_ScriptClasses.end())
            return &fullIt->second;
    }

    // Fallback: partial suffix match — allows using bare "PlayerController"
    // when the stored key is "chaineddecos.scripts.playercontroller"
    for (auto& [storedKey, type] : m_ScriptClasses)
    {
        // storedKey ends with ".<key>"
        if (storedKey.size() >= key.size() + 1)
        {
            const size_t suffixPos = storedKey.size() - key.size();
            const char dot = storedKey[suffixPos - 1];
            if (dot == '.' && storedKey.compare(suffixPos, key.size(), key) == 0)
                return &type;
        }
    }

    CH_CORE_WARN("ScriptEngine: No script found for name '{}' (key: '{}')", name, key);
    return nullptr;
}

} // namespace CHEngine