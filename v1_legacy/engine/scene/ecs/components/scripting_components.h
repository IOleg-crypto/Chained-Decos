#ifndef CD_SCENE_ECS_COMPONENTS_SCRIPTING_COMPONENTS_H
#define CD_SCENE_ECS_COMPONENTS_SCRIPTING_COMPONENTS_H

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

#endif // CD_SCENE_ECS_COMPONENTS_SCRIPTING_COMPONENTS_H
