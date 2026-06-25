#include "scriptengine_services.h"

#include "engine/app/application.h"
#include "engine/core/log.h"
#include "scripting/scriptengine.h"
#include "engine/project/project.h"
#include <Coral/GC.hpp>
#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <vector>

#include "scripting/scriptengine.h"
#include "script_glue.h"
#include "build_preset_names.h"
#include <Coral/HostInstance.hpp>

namespace Chained
{

namespace
{
constexpr const char* kGameScriptsAlcName = "GameScriptsALC";

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
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

    for (const char* preset : detail::kBuildPresetNames)
    {
        const std::filesystem::path binDir = root / "build" / preset / "bin";

        if (std::find(out.begin(), out.end(), binDir) == out.end())
        {
            out.push_back(binDir);
        }
    }
}
} // namespace

static ScriptHost s_ScriptHost;
static ScriptRegistry s_Registry;
static Scene* s_ContextScene = nullptr;

ScriptHost& GetScriptHost()
{
    return s_ScriptHost;
}

ScriptRegistry& GetScriptRegistry()
{
    return s_Registry;
}

void SetContextScene(Scene* scene)
{
    s_ContextScene = scene;
}

Scene* GetContextScene()
{
    return s_ContextScene;
}

// ScriptRuntimeSession removed

std::filesystem::path ScriptHost::ResolveCoralDirectory()
{
    std::vector<std::filesystem::path> candidateDirs;
    candidateDirs.push_back(Application::GetExecutableDirectory());
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

    const std::filesystem::path fallback = Application::GetExecutableDirectory();
    CH_CORE_WARN("ScriptEngine: Coral.Managed.dll not found in fallback candidates. Using executable directory: '{}'.",
                 fallback.string());
    for (const auto& checked : checkedPaths)
    {
        CH_CORE_TRACE("ScriptEngine: Checked '{}'", checked);
    }
    return fallback;
}

std::filesystem::path ScriptHost::ResolveCoreAssemblyPath(const std::filesystem::path& coralDir)
{
    std::vector<std::filesystem::path> coreCandidates;
    if (!coralDir.empty())
        coreCandidates.push_back(coralDir / "Chained.Managed.dll");

    const std::filesystem::path exeDir = Application::GetExecutableDirectory();
    coreCandidates.push_back(exeDir / "Chained.Managed.dll");
    coreCandidates.push_back(std::filesystem::current_path() / "Chained.Managed.dll");

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

    return std::filesystem::path("Chained.Managed.dll");
}

void ScriptHost::ClearLoadedAssemblyState()
{
    m_AppAssembly = nullptr;
    m_CoreAssembly = nullptr;
}

bool ScriptHost::Init()
{
    if (m_IsInitialized)
        return true;

    Coral::HostSettings settings;

    m_CoralDirectory = ResolveCoralDirectory();
    settings.CoralDirectory = m_CoralDirectory.string();
    CH_CORE_INFO("ScriptEngine: Using Coral directory '{}'.", m_CoralDirectory.string());

    auto status = m_Host.Initialize(settings);
    if (status != Coral::CoralInitStatus::Success)
    {
        CH_CORE_ERROR("ScriptEngine: Failed to initialize Coral! Status: {}", (int)status);
        return false;
    }

    m_IsInitialized = true;

    if (!RecreateAssemblyLoadContext(false))
    {
        m_Host.Shutdown();
        m_IsInitialized = false;
        return false;
    }

    return true;
}

void ScriptHost::Shutdown()
{
    if (!m_IsInitialized)
    {
        ClearLoadedAssemblyState();
        m_CoralDirectory.clear();
        return;
    }

    try
    {
        m_Host.UnloadAssemblyLoadContext(m_AppAssemblyContext);
        Coral::GC::Collect();
        Coral::GC::WaitForPendingFinalizers();
        Coral::GC::Collect();
        Coral::GC::Collect();
    }
    catch (const std::exception& e)
    {
        CH_CORE_WARN("ScriptEngine: Final scripting cleanup failed: {}", e.what());
    }
    catch (...)
    {
        CH_CORE_WARN("ScriptEngine: Final scripting cleanup failed (unknown exception).");
    }

    ClearLoadedAssemblyState();
    m_CoralDirectory.clear();

    m_Host.Shutdown();
    m_IsInitialized = false;
}

bool ScriptHost::RecreateAssemblyLoadContext(bool unloadCurrent)
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

bool ScriptHost::LoadAssembliesTransactional(const std::filesystem::path& appAssemblyPath)
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

    // Register internal calls (native function pointers) for both assemblies
    ScriptGlue::RegisterInternalCalls(*m_CoreAssembly);
    ScriptGlue::RegisterInternalCalls(*m_AppAssembly);

    CH_CORE_INFO("ScriptEngine: Loaded core assembly '{}'.", corePath.string());
    CH_CORE_INFO("ScriptEngine: Loaded app assembly '{}'.", appAssemblyPath.string());
    return true;
}
bool ScriptHost::LoadAppAssembly(const std::string& filepath)
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

    return LoadAssembliesTransactional(std::filesystem::path(filepath));
}

bool ScriptHost::ReloadAppAssembly(const std::string& filepath)
{
    if (!m_IsInitialized)
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called before Init().");
        return false;
    }

    if (filepath.empty())
    {
        CH_CORE_WARN("ScriptEngine::ReloadAssembly called with empty filepath.");
        return false;
    }

    if (!std::filesystem::exists(filepath))
    {
        CH_CORE_ERROR("ScriptEngine: Assembly not found at '{}'.", filepath);
        return false;
    }

    ClearLoadedAssemblyState();
    if (!RecreateAssemblyLoadContext(true))
    {
        return false;
    }

    return LoadAssembliesTransactional(std::filesystem::path(filepath));
}

void ScriptRegistry::Clear()
{
    m_ScriptClasses.clear();
    m_ShortNameToFullName.clear();
    m_MissingScriptsWarnings.clear();
}

void ScriptRegistry::Discover(Coral::ManagedAssembly& appAssembly, Coral::ManagedAssembly& coreAssembly)
{
    Clear();

    Coral::Type scriptBaseType = coreAssembly.GetLocalType("Chained.Script");

    if (!scriptBaseType)
    {
        CH_CORE_ERROR("ScriptEngine: Could not find base class 'Chained.Script' in Core assembly! Type discovery aborted.");
        return;
    }

    CH_CORE_INFO("ScriptEngine: Found base script type '{}'.", (std::string)scriptBaseType.GetFullName());

    auto types = appAssembly.GetTypes();
    CH_CORE_INFO("ScriptEngine: Discovery - searching {} types in assembly '{}'...", types.size(),
                 (std::string)appAssembly.GetName());

    for (auto& type : types)
    {
        if (*type == scriptBaseType)
            continue;

        if (!type->IsSubclassOf(scriptBaseType))
            continue;

        std::string fullName = (std::string)type->GetFullName();
        std::string key = ToLowerCopy(fullName);

        m_ScriptClasses[key] = *type;

        std::string shortKey = ToLowerCopy(GetShortTypeName(fullName));
        auto [shortIt, inserted] = m_ShortNameToFullName.emplace(shortKey, key);
        if (!inserted && shortIt->second != key)
        {
            shortIt->second.clear();
        }

        CH_CORE_INFO("ScriptEngine: Registered script '{}' (key: '{}')", fullName, key);
    }

    CH_CORE_INFO("ScriptEngine: {} script(s) registered ({} short-name keys).", m_ScriptClasses.size(),
                 m_ShortNameToFullName.size());
}

Coral::Type* ScriptRegistry::GetScriptClass(const std::string& name)
{
    std::string key = ToLowerCopy(name);

    auto it = m_ScriptClasses.find(key);
    if (it != m_ScriptClasses.end())
        return &it->second;

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

    for (auto& [storedKey, type] : m_ScriptClasses)
    {
        if (storedKey.size() >= key.size() + 1)
        {
            const size_t suffixPos = storedKey.size() - key.size();
            const char dot = storedKey[suffixPos - 1];
            if (dot == '.' && storedKey.compare(suffixPos, key.size(), key) == 0)
                return &type;
        }
    }

    if (m_MissingScriptsWarnings.find(key) == m_MissingScriptsWarnings.end())
    {
        CH_CORE_WARN("ScriptEngine: No script found for name '{}' (key: '{}')", name, key);
        m_MissingScriptsWarnings.insert(key);
    }
    return nullptr;
}

} // namespace Chained