#include "ScriptManager.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "editor/logic/ISceneManager.h"

namespace CHEngine
{

ScriptManager::ScriptManager()
{
}

ScriptManager::~ScriptManager()
{
}

bool ScriptManager::Initialize()
{
    CD_CORE_INFO("Initializing Scripting System (Lua)...");

    try
    {
        m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string,
                             sol::lib::table, sol::lib::os);
        BindEngineAPI();
        m_initialized = true;
        CD_CORE_INFO("Scripting System initialized successfully.");
        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("Failed to initialize Scripting System: %s", e.what());
        return false;
    }
}

void ScriptManager::Shutdown()
{
    CD_CORE_INFO("Shutting down Scripting System...");
    m_initialized = false;
}

void ScriptManager::Update(float deltaTime)
{
    if (!m_initialized)
        return;

    // TODO: Handle global script updates or hot-reloading checks here
}

bool ScriptManager::RunScript(const std::string &path)
{
    if (!m_initialized)
        return false;

    auto result = m_lua.script_file(path, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        CD_CORE_ERROR("Lua Script Error (%s): %s", path.c_str(), err.what());
        return false;
    }
    return true;
}

bool ScriptManager::RunString(const std::string &code)
{
    if (!m_initialized)
        return false;

    auto result = m_lua.script(code, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        CD_CORE_ERROR("Lua String Error: %s", err.what());
        return false;
    }
    return true;
}

void ScriptManager::BindEngineAPI()
{
    // 1. Logging
    m_lua.set_function("LogInfo", [](const std::string &msg) { CD_INFO("[Lua] %s", msg.c_str()); });
    m_lua.set_function("LogWarn", [](const std::string &msg) { CD_WARN("[Lua] %s", msg.c_str()); });
    m_lua.set_function("LogError",
                       [](const std::string &msg) { CD_ERROR("[Lua] %s", msg.c_str()); });

    // 2. Bind Scene and UI APIs
    BindSceneAPI();
    BindUIAPI();
}

void ScriptManager::SetSceneManager(ISceneManager *sceneManager)
{
    m_sceneManager = sceneManager;
    CD_CORE_INFO("ScriptManager: Scene Manager set.");
}

void ScriptManager::RegisterButtonCallback(const std::string &buttonName, sol::function callback)
{
    m_buttonCallbacks[buttonName] = std::move(callback);
    CD_CORE_INFO("ScriptManager: Registered callback for button '%s'", buttonName.c_str());
}

void ScriptManager::BindSceneAPI()
{
    // Scene Management API
    m_lua.set_function("LoadScene",
                       [this](const std::string &scenePath)
                       {
                           if (!m_sceneManager)
                           {
                               CD_ERROR("[Lua] LoadScene failed: SceneManager not set!");
                               return;
                           }
                           CD_INFO("[Lua] Loading scene: %s", scenePath.c_str());
                           m_sceneManager->LoadScene(scenePath);
                       });

    m_lua.set_function("QuitGame",
                       []()
                       {
                           CD_INFO("[Lua] Quit game requested.");
                           Engine::Instance().RequestExit();
                       });
}

void ScriptManager::BindUIAPI()
{
    // UI Button Callback Registration
    m_lua.set_function("OnButtonClick",
                       [this](const std::string &buttonName, sol::function callback)
                       { RegisterButtonCallback(buttonName, std::move(callback)); });

    // UI Helper to trigger button callbacks (called from C++)
    m_lua.set_function("TriggerButtonCallback",
                       [this](const std::string &buttonName)
                       {
                           auto it = m_buttonCallbacks.find(buttonName);
                           if (it != m_buttonCallbacks.end())
                           {
                               try
                               {
                                   it->second(); // Call Lua function
                               }
                               catch (const std::exception &e)
                               {
                                   CD_ERROR("[Lua] Button callback error for '%s': %s",
                                            buttonName.c_str(), e.what());
                               }
                           }
                           else
                           {
                               CD_WARN("[Lua] No callback registered for button '%s'",
                                       buttonName.c_str());
                           }
                       });
}

} // namespace CHEngine
