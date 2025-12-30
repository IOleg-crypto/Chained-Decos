#include "ScriptEngine.h"
#include "core/Engine.h"
#include "core/scripting/ScriptManager.h"

namespace CHEngine
{

bool ScriptEngine::RunScript(const std::string &path)
{
    return Engine::Instance().GetScriptManager().RunScript(path);
}

bool ScriptEngine::RunString(const std::string &code)
{
    return Engine::Instance().GetScriptManager().RunString(code);
}

sol::state &ScriptEngine::GetLuaState()
{
    return Engine::Instance().GetScriptManager().GetLuaState();
}

void ScriptEngine::RegisterButtonCallback(const std::string &buttonName, sol::function callback)
{
    Engine::Instance().GetScriptManager().RegisterButtonCallback(buttonName, callback);
}

} // namespace CHEngine
