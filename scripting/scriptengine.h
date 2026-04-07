#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "engine/core/base.h"
#include <Coral/Assembly.hpp>
#include <Coral/HostInstance.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Scene;

// ─────────────────────────────────────────────────────────────────────────────
//  ScriptEngine
//
//  Manages the .NET (CoreCLR) host via Coral and handles:
//    - Host init / shutdown
//    - Assembly load / hot-reload
//    - Script class discovery and lookup
//
//  The class is fully static (singleton-style) because the host is a process-
//  level singleton in CoreCLR.
// ─────────────────────────────────────────────────────────────────────────────
class CH_API ScriptEngine
{
public:
    // ── Lifecycle ────────────────────────────────────────────────────────
    ScriptEngine();
    ~ScriptEngine();

    static void Init();
    static void Shutdown();

    void InternalInit();
    void InternalShutdown();

    // ── Assembly management ──────────────────────────────────────────────
    /// Load (or re-load) the game script DLL.
    bool LoadAppAssembly(const std::string& filepath);
    /// Hot-reload: stops running scripts, unloads the old ALC, loads the new DLL.
    bool ReloadAssembly();

    // ── Script type lookup ───────────────────────────────────────────────
    /// Returns a pointer to the Coral::Type for the given short or full class name.
    /// Search is case-insensitive. Returns nullptr if not found.
    Coral::Type* GetScriptClass(const std::string& name);

    /// All discovered script types keyed by lowercase full name.
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const
    {
        return m_ScriptClasses;
    }

    // ── Accessors ────────────────────────────────────────────────────────
    bool IsInitialized() const
    {
        return m_IsInitialized;
    }
    bool IsReloadInProgress() const
    {
        return m_ReloadInProgress;
    }
    Scene* GetActiveScene() const
    {
        return m_ActiveScene;
    }
    void SetActiveScene(Scene* scene)
    {
        m_ActiveScene = scene;
    }

    /// Called from C# script glue — queue a scene to load next frame.
    void RequestLoadScene(const std::string& path)
    {
        m_PendingScenePath = path;
    }
    /// Consumed by RuntimeLayer::OnUpdate each frame. Returns the path and clears it.
    std::string ConsumeRequestedScene()
    {
        std::string s = m_PendingScenePath;
        m_PendingScenePath.clear();
        return s;
    }

    static ScriptEngine& Get();

private:
    void DiscoverScriptTypes();
    void ClearLoadedAssemblyState();
    bool RecreateAssemblyLoadContext(bool unloadCurrent);
    bool LoadAssembliesTransactional(const std::filesystem::path& appAssemblyPath);

private:
    Scene* m_ActiveScene = nullptr;
    std::filesystem::path m_CoralDirectory;
    Coral::HostInstance m_Host;
    Coral::AssemblyLoadContext m_AppAssemblyContext;
    Coral::ManagedAssembly* m_AppAssembly = nullptr;
    Coral::ManagedAssembly* m_CoreAssembly = nullptr;

    std::unordered_map<std::string, Coral::Type> m_ScriptClasses;
    std::unordered_map<std::string, std::string> m_ShortNameToFullName;
    bool m_IsInitialized = false;
    bool m_ReloadInProgress = false;
    std::string m_PendingScenePath;
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H