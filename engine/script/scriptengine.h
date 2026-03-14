#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include <filesystem>
#include <string>
#include <unordered_map>
#include <Coral/HostInstance.hpp>
#include <Coral/Assembly.hpp>

namespace CHEngine {

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
class ScriptEngine
{
public:
    // ── Lifecycle ────────────────────────────────────────────────────────
    ScriptEngine();
    ~ScriptEngine();

    void Init();
    void Shutdown();

    // ── Assembly management ──────────────────────────────────────────────
    /// Load (or re-load) the game script DLL.
    void LoadAppAssembly(const std::string& filepath);
    /// Hot-reload: stops running scripts, unloads the old ALC, loads the new DLL.
    void ReloadAssembly();

    // ── Script type lookup ───────────────────────────────────────────────
    /// Returns a pointer to the Coral::Type for the given short or full class name.
    /// Search is case-insensitive. Returns nullptr if not found.
    Coral::Type* GetScriptClass(const std::string& name);

    /// All discovered script types keyed by lowercase full name.
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const
    { return m_ScriptClasses; }

    // ── Accessors ────────────────────────────────────────────────────────
    bool   IsInitialized() const { return m_IsInitialized; }
    Scene* GetActiveScene() const { return m_ActiveScene; }
    void   SetActiveScene(Scene* scene) { m_ActiveScene = scene; }

    static ScriptEngine& Get();

private:
    void DiscoverScriptTypes();

private:
    Scene*                                     m_ActiveScene = nullptr;
    Coral::HostInstance                        m_Host;
    Coral::AssemblyLoadContext                 m_AppAssemblyContext;
    Coral::ManagedAssembly*                    m_AppAssembly = nullptr;
    Coral::ManagedAssembly*                    m_CoreAssembly = nullptr;

    // Keyed by lowercase full qualified name, e.g. "chaineddecos.scripts.playercontroller"
    std::unordered_map<std::string, Coral::Type> m_ScriptClasses;
    bool m_IsInitialized = false;
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H