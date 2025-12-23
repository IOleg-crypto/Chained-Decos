#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H

#include <string>
#include <vector>

namespace CHEngine
{

struct ScriptComponent
{
    std::string scriptPath; // Relative to project root
    bool isEnabled = true;

    // Hot-reloading state
    long long lastModified = 0;
};

} // namespace CHEngine

#endif // SCRIPT_COMPONENT_H
