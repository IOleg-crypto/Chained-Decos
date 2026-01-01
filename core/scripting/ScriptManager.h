#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <entt/entt.hpp>
#include <memory>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>


namespace CHEngine
{

class ScriptManager
{
public:
    ScriptManager();
    ~ScriptManager();

    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static void Update(float deltaTime);

    static ScriptManager &Get()
    {
        return *s_Instance;
    }

    // Lua State Access
    static sol::state &GetLuaState();

    // Script execution
    static bool RunScript(const std::string &path);
    static bool RunString(const std::string &code);

    // Entity Script Lifecycle (Hazel Style)
    static void InitializeScripts(entt::registry &registry);
    static void UpdateScripts(entt::registry &registry, float deltaTime);

    static void SetActiveRegistry(entt::registry *registry);

    // Register UI button callback
    static void RegisterButtonCallback(const std::string &buttonName, sol::function callback);

private:
    void BindEngineAPI();
    void BindSceneAPI();
    void BindUIAPI();
    void BindAudioAPI();
    void BindGameplayAPI();

    bool InternalInitialize();
    void InternalShutdown();
    void InternalUpdate(float deltaTime);
    void InternalInitializeScripts(entt::registry &registry);
    void InternalUpdateScripts(entt::registry &registry, float deltaTime);
    bool InternalRunScript(const std::string &path);
    bool InternalRunString(const std::string &code);
    void InternalRegisterButtonCallback(const std::string &buttonName, sol::function callback);

    // Internal Lifecycle Callers
    void CallLuaFunction(const std::string &scriptPath, const std::string &functionName,
                         uint32_t entityId, float dt = 0.0f);

private:
    sol::state m_lua;
    bool m_initialized = false;
    entt::registry *m_activeRegistry = nullptr;
    std::unordered_map<std::string, sol::function> m_buttonCallbacks;

    static std::unique_ptr<ScriptManager> s_Instance;
};

} // namespace CHEngine

#endif // SCRIPT_MANAGER_H
