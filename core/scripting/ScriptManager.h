#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

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

    bool Initialize();
    void Shutdown();

    void Update(float deltaTime);

    // Lua State Access
    sol::state &GetLuaState()
    {
        return m_lua;
    }

    // Script execution
    bool RunScript(const std::string &path);
    bool RunString(const std::string &code);

    // Entity Script Lifecycle (Hazel Style)
    void InitializeScripts();
    void UpdateScripts(float deltaTime);

    // Deprecated for now, ScriptManager should use Engine services directly
    void SetSceneManager(void *unused);

    // Register UI button callback
    void RegisterButtonCallback(const std::string &buttonName, sol::function callback);

private:
    void BindEngineAPI();
    void BindSceneAPI();
    void BindUIAPI();
    void BindGameplayAPI();

    // Internal Lifecycle Callers
    void CallLuaFunction(const std::string &scriptPath, const std::string &functionName,
                         uint32_t entityId, float dt = 0.0f);

private:
    sol::state m_lua;
    bool m_initialized = false;
    std::unordered_map<std::string, sol::function> m_buttonCallbacks;
};

} // namespace CHEngine

#endif // SCRIPT_MANAGER_H
