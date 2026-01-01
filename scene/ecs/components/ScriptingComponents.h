#ifndef SCRIPTING_COMPONENTS_H
#define SCRIPTING_COMPONENTS_H

#include <string>

namespace CHEngine
{

/**
 * @brief Component that links an entity to a C# script class.
 */
struct CSharpScriptComponent
{
    std::string className; // Full name: Namespace.Class
    bool initialized = false;

    // Pointer to the managed instance (GCHandle)
    void *handle = nullptr;
};

} // namespace CHEngine

#endif // SCRIPTING_COMPONENTS_H
