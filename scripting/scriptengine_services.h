
#ifndef CH_SCRIPT_ENGINE_SERVICES_H
#define CH_SCRIPT_ENGINE_SERVICES_H

#include <Coral/Assembly.hpp>
#include <Coral/HostInstance.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace CHEngine
{

class Scene;

// Runtime scripting context (used by glue)
void SetContextScene(Scene* scene);
Scene* GetContextScene();

// Owns the CoreCLR host and the current app/core assembly handles.
class ScriptHost
{
public:
    bool Init();
    void Shutdown();

    bool LoadAppAssembly(const std::string& filepath);
    bool ReloadAppAssembly(const std::string& filepath);

    bool IsInitialized() const
    {
        return m_IsInitialized;
    }

    bool IsReloadInProgress() const
    {
        return m_ReloadInProgress;
    }

    void SetReloadInProgress(bool inProgress)
    {
        m_ReloadInProgress = inProgress;
    }

    Coral::ManagedAssembly* GetAppAssembly() const
    {
        return m_AppAssembly;
    }

    Coral::ManagedAssembly* GetCoreAssembly() const
    {
        return m_CoreAssembly;
    }

    void ClearLoadedAssemblyState();

private:
    bool RecreateAssemblyLoadContext(bool unloadCurrent);
    bool LoadAssembliesTransactional(const std::filesystem::path& appAssemblyPath);

    static std::filesystem::path ResolveCoralDirectory();
    static std::filesystem::path ResolveCoreAssemblyPath(const std::filesystem::path& coralDir);

private:
    std::filesystem::path m_CoralDirectory;
    Coral::HostInstance m_Host;
    Coral::AssemblyLoadContext m_AppAssemblyContext;
    Coral::ManagedAssembly* m_AppAssembly = nullptr;
    Coral::ManagedAssembly* m_CoreAssembly = nullptr;
    bool m_IsInitialized = false;
    bool m_ReloadInProgress = false;
};

// Caches discovered script types and resolves short names to full names.
class ScriptRegistry
{
public:
    void Clear();
    void Discover(Coral::ManagedAssembly& appAssembly, Coral::ManagedAssembly& coreAssembly);
    Coral::Type* GetScriptClass(const std::string& name);

    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const
    {
        return m_ScriptClasses;
    }

private:
    std::unordered_map<std::string, Coral::Type> m_ScriptClasses;
    std::unordered_map<std::string, std::string> m_ShortNameToFullName;
    std::unordered_set<std::string> m_MissingScriptsWarnings;
};

} // namespace CHEngine

#endif // CH_SCRIPT_ENGINE_SERVICES_H

