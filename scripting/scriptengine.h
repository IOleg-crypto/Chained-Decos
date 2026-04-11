#ifndef CH_SCRIPT_ENGINE_H
#define CH_SCRIPT_ENGINE_H

#include "engine/core/base.h"
#include "scriptengine_services.h"
#include <Coral/Assembly.hpp>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class Scene;

// ─────────────────────────────────────────────────────────────────────────────
//  ScriptEngine
//
//  Thin singleton facade over the scripting host, type registry, and runtime
//  scene handoff state.
//
//  The singleton shape stays because CoreCLR and the runtime/editor layers
//  expect a process-wide scripting service.
// ─────────────────────────────────────────────────────────────────────────────
class ScriptEngine
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
    /// UI-safe reload entry point with consistent guard/log behavior.
    bool RequestAssemblyReload(const char* requestSource);

    // ── Script type lookup ───────────────────────────────────────────────
    /// Returns a pointer to the Coral::Type for the given short or full class name.
    /// Search is case-insensitive. Returns nullptr if not found.
    Coral::Type* GetScriptClass(const std::string& name);

    /// All discovered script types keyed by lowercase full name.
    const std::unordered_map<std::string, Coral::Type>& GetScriptClasses() const
    {
        return GetScriptTypeRegistry().GetScriptClasses();
    }

    // ── Accessors ────────────────────────────────────────────────────────
    bool IsInitialized() const
    {
        return GetScriptAssemblyHost().IsInitialized();
    }
    bool IsReloadInProgress() const
    {
        return m_RuntimeSession.IsReloadInProgress();
    }
    bool CanExecuteFrameScripts() const
    {
        return GetScriptAssemblyHost().IsInitialized() && !m_RuntimeSession.IsReloadInProgress();
    }
    Scene* GetActiveScene() const
    {
        return m_RuntimeSession.GetActiveScene();
    }
    void SetActiveScene(Scene* scene)
    {
        m_RuntimeSession.SetActiveScene(scene);
    }

    /// Called from C# script glue — queue a scene to load next frame.
    void RequestLoadScene(const std::string& path)
    {
        m_RuntimeSession.RequestLoadScene(path);
    }
    /// Consumed by RuntimeLayer::OnUpdate each frame. Returns the path and clears it.
    std::string ConsumeRequestedScene()
    {
        return m_RuntimeSession.ConsumeRequestedScene();
    }
    /// Safely consume pending scene requests for frame updates.
    /// Returns false if reload is in progress or there is no pending path.
    bool TryConsumeRequestedScene(std::string& outPath)
    {
        return m_RuntimeSession.TryConsumeRequestedScene(outPath);
    }

    static ScriptEngine& Get();

private:
    ScriptRuntimeSession m_RuntimeSession;
};

} // namespace CHEngine
#endif // CH_SCRIPT_ENGINE_H
