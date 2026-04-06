#include "scriptengine.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "scene_scripting.h"
#include "engine/core/filesystem_utils.h"
#include <Coral/ManagedObject.hpp>
#include <algorithm>
#include <filesystem>
#include <vector>
#include "script_glue.h"

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
    if (m_IsInitialized)
        return;

    CH_CORE_INFO("ScriptEngine: Initializing CoreCLR...");

    Coral::HostSettings settings;

    // Coral looks for Coral.Managed.dll in this directory. Resolve a robust fallback chain
    // because runtime may be launched from different working directories.
    std::vector<std::filesystem::path> coralDirCandidates;
    coralDirCandidates.push_back(FilesystemUtils::GetExecutableDirectory());
    coralDirCandidates.push_back(std::filesystem::current_path());

#ifdef PROJECT_ROOT_DIR
    coralDirCandidates.push_back(std::filesystem::path(PROJECT_ROOT_DIR) / "build" / "windows-ninja" / "bin");
    coralDirCandidates.push_back(std::filesystem::path(PROJECT_ROOT_DIR));
#endif

    if (auto project = Project::GetActive())
    {
        coralDirCandidates.push_back(project->GetConfig().ProjectDirectory / "build" / "windows-ninja" / "bin");
        coralDirCandidates.push_back(project->GetConfig().ProjectDirectory);
    }

    std::filesystem::path selectedCoralDir;
    std::vector<std::string> checkedPaths;

    for (const auto& candidateRaw : coralDirCandidates)
    {
        if (candidateRaw.empty())
        {
            continue;
        }

        std::error_code ec;
        std::filesystem::path candidate = std::filesystem::absolute(candidateRaw, ec).lexically_normal();
        if (ec)
        {
            candidate = candidateRaw;
        }

        const std::filesystem::path managedPath = candidate / "Coral.Managed.dll";
        checkedPaths.push_back(managedPath.string());

        if (std::filesystem::exists(managedPath))
        {
            selectedCoralDir = candidate;
            break;
        }
    }

    if (selectedCoralDir.empty())
    {
        selectedCoralDir = FilesystemUtils::GetExecutableDirectory();
        CH_CORE_WARN("ScriptEngine: Coral.Managed.dll not found in fallback candidates. Using executable directory: '{}'",
                     selectedCoralDir.string());
        for (const auto& checked : checkedPaths)
        {
            CH_CORE_WARN("ScriptEngine: Checked '{}'", checked);
        }
    }
    else
    {
        CH_CORE_INFO("ScriptEngine: Using Coral directory '{}'", selectedCoralDir.string());
    }

    m_CoralDirectory = selectedCoralDir;
    settings.CoralDirectory = selectedCoralDir.string();

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
    m_ScriptClasses.clear();
    m_AppAssembly = nullptr;
    m_CoreAssembly = nullptr;
    m_CoralDirectory.clear();
    m_Host.Shutdown();
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
        std::vector<std::filesystem::path> coreCandidates;
        if (!m_CoralDirectory.empty())
        {
            coreCandidates.push_back(m_CoralDirectory / "CHEngine.Managed.dll");
        }
        coreCandidates.push_back(FilesystemUtils::GetExecutableDirectory() / "CHEngine.Managed.dll");
        coreCandidates.push_back(std::filesystem::current_path() / "CHEngine.Managed.dll");
        coreCandidates.push_back("CHEngine.Managed.dll");

        std::filesystem::path corePath;
        for (const auto& candidate : coreCandidates)
        {
            if (candidate.is_relative())
            {
                corePath = candidate;
                continue;
            }

            if (std::filesystem::exists(candidate))
            {
                corePath = candidate;
                break;
            }
        }

        if (corePath.empty())
        {
            corePath = "CHEngine.Managed.dll";
        }

        // Load the core assembly directly — no shadow copy needed
        m_CoreAssembly = &m_AppAssemblyContext.LoadAssembly(corePath.string());
        if (!m_CoreAssembly || m_CoreAssembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
        {
            CH_CORE_ERROR("ScriptEngine: Failed to load core assembly '{}'. Status: {}", 
                          corePath.string(), m_CoreAssembly ? (int)m_CoreAssembly->GetLoadStatus() : -1);
            m_CoreAssembly = nullptr;
            return;
        }
        CH_CORE_INFO("ScriptEngine: Loaded core assembly '{}'.", corePath.string());

        // 2. Load the game scripts directly from the project directory
        m_AppAssembly = &m_AppAssemblyContext.LoadAssembly(filepath);

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

    // 3. Fresh ALC + load the new DLL directly from the project
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
        std::string key = fullName;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        m_ScriptClasses[key] = *type;
        CH_CORE_INFO("ScriptEngine: Registered script '{}' (key: '{}')", fullName, key);
    }

    CH_CORE_INFO("ScriptEngine: {} script(s) registered.", m_ScriptClasses.size());
}

// ── Script lookup ─────────────────────────────────────────────────────────────
Coral::Type* ScriptEngine::GetScriptClass(const std::string& name)
{
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

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