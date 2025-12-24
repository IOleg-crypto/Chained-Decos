#ifndef SCRIPTING_COMPONENTS_H
#define SCRIPTING_COMPONENTS_H

#include <string>

namespace CHEngine
{

/**
 * @brief Component that links an entity to a Lua script.
 *
 * In Hazel-style architecture, the engine calls OnInit and OnUpdate
 * for any entity possessing this component.
 */
struct LuaScriptComponent
{
    std::string scriptPath; // Path to the .lua file
    bool initialized = false;

    // Properties that can be passed from the editor to Lua
    // (Future enhancement: std::map<std::string, std::string> properties)
};

} // namespace CHEngine

#endif // SCRIPTING_COMPONENTS_H
