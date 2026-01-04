#ifndef CH_SCRIPTING_COMPONENTS_H
#define CH_SCRIPTING_COMPONENTS_H

#include <string>

namespace CH
{
/**
 * @brief Component that links an entity to a C# script class.
 */
struct CSharpScriptComponent
{
    std::string ClassName; // Full name: Namespace.Class
    bool Initialized = false;

    // Pointer to the managed instance (GCHandle)
    void *Handle = nullptr;

    CSharpScriptComponent() = default;
    CSharpScriptComponent(const CSharpScriptComponent &) = default;
    CSharpScriptComponent(const std::string &className) : ClassName(className)
    {
    }
};
} // namespace CH

#endif // CH_SCRIPTING_COMPONENTS_H
