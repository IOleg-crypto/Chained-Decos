#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <sol/sol.hpp>
#include <string>

namespace CHEngine
{

class ScriptEngine
{
public:
    static bool RunScript(const std::string &path);
    static bool RunString(const std::string &code);

    static sol::state &GetLuaState();

    // UI Integration
    static void RegisterButtonCallback(const std::string &buttonName, sol::function callback);
};

} // namespace CHEngine

#endif // SCRIPT_ENGINE_H
