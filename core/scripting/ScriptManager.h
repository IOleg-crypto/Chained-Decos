#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>

class ISceneManager;

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

    // Set scene manager for scene switching API
    void SetSceneManager(::ISceneManager *sceneManager);

    // Register UI button callback
    void RegisterButtonCallback(const std::string &buttonName, sol::function callback);

private:
    void BindEngineAPI();
    void BindSceneAPI();
    void BindUIAPI();

private:
    sol::state m_lua;
    bool m_initialized = false;
    ::ISceneManager *m_sceneManager = nullptr;
    std::unordered_map<std::string, sol::function> m_buttonCallbacks;
};

} // namespace CHEngine

#endif // SCRIPT_MANAGER_H
