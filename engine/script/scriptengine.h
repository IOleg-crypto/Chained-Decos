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
    static void Init();
    static void Shutdown();

    // ── Assembly management ──────────────────────────────────────────────
    /// Load (or re-load) the game script DLL.
    static void LoadAppAssembly(const std::string& filepath);
    /// Hot-reload: stops running scripts, unloads the old ALC, loads the new DLL.
    static void ReloadAssembly();

    // ── Script type lookup ───────────────────────────────────────────────
    /// Returns a pointer to the Coral::Type for the given short or full class name.
    /// Search is case-insensitive. Returns nullptr if not found.
    static Coral::Type* GetScriptClass(const std::string& name);

    /// All discovered script types keyed by lowercase full name.
    static const std::unordered_map<std::string, Coral::Type>& GetScriptClasses()
    { return s_ScriptClasses; }

    // ── Accessors ────────────────────────────────────────────────────────
    static bool   IsInitialized()  { return s_IsInitialized; }
    static Scene* GetActiveScene() { return s_ActiveScene; }
    static void   SetActiveScene(Scene* scene) { s_ActiveScene = scene; }

private:
    static void DiscoverScriptTypes();

private:
    static Scene*                                     s_ActiveScene;
    static Coral::HostInstance                        s_Host;
    static Coral::AssemblyLoadContext                 s_AppAssemblyContext;
    static Coral::ManagedAssembly*                    s_AppAssembly;
    static Coral::ManagedAssembly*                    s_CoreAssembly;

    // Keyed by lowercase full qualified name, e.g. "chaineddecos.scripts.playercontroller"
    static std::unordered_map<std::string, Coral::Type> s_ScriptClasses;
    static bool s_IsInitialized;
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H